#! /usr/bin/python

"""
Generates XMLs for test Fusion assets.

Currently, only an imagery project and imagery assets XMLs are generated.

WARNING: This scripts generates 1,000,000 asset directories.  This may make
file operations in the test assets directory slow.
"""

import argparse
import functools
import jinja2
import math
import os
import os.path
import sys
import uuid


# Create some decorators for methods that serve as tasks to be executed.
def execute_once(func):
    """Make a function exectule only once.  Calls after the first one return
    immediately.
    """

    @functools.wraps(func)
    def wrapper_execute_once(*args, **kwargs):
        if wrapper_execute_once.executed:
            return

        res = func(*args, **kwargs)
        wrapper_execute_once.executed = True

        return res

    wrapper_execute_once.executed = False

    return wrapper_execute_once


class ProgressMeter(object):
    """A basic interface for reporting task progress."""

    def __init__(self, start_value, end_value, update_interval=1000):
        """
            update_interval = How much the progress value should change before
        the display is updated.
        """

        self.start_value = start_value
        self.end_value = end_value
        self.value = start_value
        self.update_interval = update_interval
        self.display_last_value = None

    def advance(self, progress_amount):
        self.value += progress_amount
        self.update_progress()

    def close(self):
        sys.stdout.write(os.linesep)

    def update_progress(self):
        if (self.value - self.start_value) % self.update_interval == 0:
            self.update_display()

    def update_display(self):
        remaining_ratio = float(self.end_value - self.value) / \
            (self.end_value - self.start_value)
        display_value = math.floor(remaining_ratio * 10.0)
        if display_value != self.display_last_value:
            sys.stdout.write('{0:.0f}'.format(display_value))
            self.display_last_value = display_value
        else:
            sys.stdout.write('.')
        sys.stdout.flush()

class AssetXmlWriter(object):
    def __init__(self, templates_dir):
        self.template_engine = jinja2.Environment(
            loader = jinja2.FileSystemLoader(templates_dir),
            trim_blocks = True)

    def write_xml_template(self, output_path, template_name, vars):
        output_dir = os.path.dirname(output_path)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        with open(output_path, 'w') as f:
            f.write(self.template_engine.get_template(template_name).render(
                **vars))

class ImageInset(object):
    def __init__(self, data_asset, max_level=14, override_max=False):
        self.data_asset = data_asset
        self.max_level = max_level
        self.override_max = 1 if override_max else 0
        self.data_asset_name = data_asset.name

class AssetSource(object):
    def __init__(self, uri, size, mtime):
        self.uri = uri
        self.size = size
        self.mtime = mtime

class VersionedAsset(object):
    def __init__(self, name=None):
        if name is not None:
            self.name = name
        self.input_assets = []
        self.asset_versions = []

    def add_version(self, asset_version):
        """Called automatically when a new asset version for this asset is
        construted."""

        self.asset_versions.append(asset_version)

    def output(self, asset_root, xml_writer):
        for version in self.asset_versions:
            version.output(asset_root, xml_writer)
        self.output_xml(asset_root, xml_writer)

    def _output_xml(self, asset_root, xml_writer, template_dir, vars):
        """Outputs the XML for only this asset.

        Use output() to write the XMLs for this asset, all of its versions,
        and asset components.
        """

        template_vars = {
            'name': self.name,
            'inputs': [v.name for v in self.input_assets],
            'versions': [v.name for v in self.asset_versions]
        }
        template_vars.update(vars)

        xml_writer.write_xml_template(
            os.path.join(asset_root, self.name, 'khasset.xml'),
            os.path.join(template_dir, 'khasset.xml.j2'),
            vars = template_vars)

class AssetVersion(object):
    def __init__(self, asset, version):
        self.asset = asset
        self.version = version
        self.state = 'Succeeded'
        self.input_asset_versions = []
        self.child_asset_versions = []
        self.listener_asset_versions = []
        asset.add_version(self)

    @property
    def name(self):
        return '{0}?version={1}'.format(self.asset.name, self.version)

    def output(self, asset_root, xml_writer):
        self.output_xml(asset_root, xml_writer)

    def _output_xml(self, asset_root, xml_writer, template_dir, vars):
        template_vars = {
            'name': self.name,
            'version': self.version,
            'state': self.state,
            'inputs': [v.name for v in self.input_asset_versions],
            'children': [v.name for v in self.child_asset_versions],
            'listeners': [v.name for v in self.listener_asset_versions]
        }
        template_vars.update(vars)

        xml_writer.write_xml_template(
            os.path.join(
                asset_root, self.asset.name,
                'ver{0:03}'.format(self.version), 'khassetver.xml'),
            os.path.join(template_dir, 'khassetver.xml.j2'),
            vars = template_vars
        )

class RasterProjectAsset(VersionedAsset):
    def __init__(self, name):
        super(RasterProjectAsset, self).__init__(name)
        self.insets = []
        self.asset_uuid = uuid.uuid4()

    def add_inset(self, valid_raster_product_asset):
        assert isinstance(valid_raster_product_asset, RasterProductAsset)

        self.input_assets.append(valid_raster_product_asset)
        self.insets.append(valid_raster_product_asset)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(asset_root, xml_writer, 'RasterProject',
            vars = {
                'insets': self.insets,
                'asset_uuid': self.asset_uuid
            })

class RasterProjectAssetVersion(AssetVersion):
    def __init__(self, asset, version):
        assert isinstance(asset, RasterProjectAsset)

        super(RasterProjectAssetVersion, self).__init__(asset, version)
        self.insets = []

    def add_inset(self, inset, inset_version):
        assert isinstance(inset, ImageInset)

        self.insets.append(inset)
        self.input_asset_versions.append(inset_version)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(asset_root, xml_writer, 'RasterProject',
            vars = {
                'insets': self.insets,
                'asset_uuid': uuid.uuid4()                
            }
        )

class RasterProductAsset(VersionedAsset):
    def __init__(self, name):
        super(RasterProductAsset, self).__init__(name)
        self.source = None
        self.combined_rp = None

    def output(self, asset_root, xml_writer):
        if self.source is not None:
            self.source.output(asset_root, xml_writer)
        if self.combined_rp is not None:
            self.combined_rp.output(asset_root, xml_writer)
        super(RasterProductAsset, self).output(asset_root, xml_writer)

    def output_xml(self, asset_root, xml_writer):
        inputs = [v.name for v in self.input_assets]
        if self.source is not None:
            inputs.append(self.source.name)
        self._output_xml(asset_root, xml_writer, 'RasterProduct',
            vars = {
                'inputs': inputs,
            })

class RasterProductAssetVersion(AssetVersion):
    def __init__(self, asset, version):
        assert isinstance(asset, RasterProductAsset)

        super(RasterProductAssetVersion, self).__init__(asset, version)

    def add_input(self, asset_version):
        self.input_asset_versions.append(asset_version)
    
    def add_child(self, asset_version):
        self.child_asset_versions.append(asset_version)

    def add_listener(self, asset_version):
        self.listener_asset_versions.append(asset_version)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(asset_root, xml_writer, 'RasterProduct', vars = {})

class RasterProductSourceAsset(VersionedAsset):
    def __init__(self, asset):
        assert isinstance(asset, RasterProductAsset)

        self.asset = asset
        super(RasterProductSourceAsset, self).__init__()

        self.sources = []
        asset.source = self

    @property
    def name(self):
        return os.path.join(self.asset.name, 'source.kisource')

    def add_source(self, source):
        assert isinstance(source, AssetSource)

        self.sources.append(source)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(
            asset_root, xml_writer,
            os.path.join('RasterProduct', 'source.kisource'),
            vars = {
                'sources': self.sources
            }
        )

class RasterProductSourceAssetVersion(AssetVersion):
    def __init__(self, asset, version):
        assert isinstance(asset, RasterProductSourceAsset)

        super(RasterProductSourceAssetVersion, self).__init__(asset, version)
        self.out_files = []
        self.sources = []

    def add_out_file(self, path):
        self.out_files.append(path)

    def add_source(self, source):
        assert isinstance(source, AssetSource)

        self.sources.append(source)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(
            asset_root, xml_writer,
            os.path.join('RasterProduct', 'source.kisource'),
            vars = {
                'outfiles': self.out_files,
                'sources': self.sources
            })

class RasterProductCombinedRpAsset(VersionedAsset):
    def __init__(self, asset):
        assert isinstance(asset, RasterProductAsset)

        self.asset = asset
        super(RasterProductCombinedRpAsset, self).__init__()

        self.sources = []
        asset.combined_rp = self

    @property
    def name(self):
        return os.path.join(self.asset.name, 'CombinedRP.kia')

    def add_source(self, source):
        assert isinstance(source, AssetSource)

        self.sources.append(source)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(
            asset_root, xml_writer,
            os.path.join('RasterProduct', 'CombinedRP.kia'),
            vars = {}
        )

class RasterProductCombinedRpAssetVersion(AssetVersion):
    def __init__(self, asset, version):
        assert isinstance(asset, RasterProductCombinedRpAsset)

        super(RasterProductCombinedRpAssetVersion, self).__init__(
            asset, version)
        self.parent_asset_versions = []
        self.out_files = []
        self.sources = []

    def add_parent(self, parent):
        self.parent_asset_versions.append(parent)

    def add_out_file(self, path):
        self.out_files.append(path)

    def add_source(self, source):
        assert isinstance(source, AssetSource)

        self.sources.append(source)

    def output_xml(self, asset_root, xml_writer):
        self._output_xml(
            asset_root, xml_writer,
            os.path.join('RasterProduct', 'CombinedRP.kia'),
            vars = {
                'parents': [v.name for v in self.parent_asset_versions],
                'outfiles': self.out_files,
                'sources': self.sources
            })


class TestAssetGeneratorBase(object):
    def __init__(self):
        # Capture our current directory:
        self_dir = os.path.dirname(os.path.abspath(__file__))

        self.asset_root = os.path.join(
            self_dir, 'fusion', 'testdata', 'gevol', 'assets')
        self.templates_dir = os.path.join(
            self_dir, 'fusion', 'testdata', 'assets', 'templates')
        self.xml_writer = AssetXmlWriter(self.templates_dir)
        self.valid_raster_project_count = 1000000
        self.progress_meter = ProgressMeter(0, 2)

    def get_asset_basename(self, asset_number, asset_suffix):
        return 'asset-{0:07}{1}'.format(asset_number, asset_suffix)


class TestValidAssetGenerator(TestAssetGeneratorBase):
    def __init__(self):
        super(TestValidAssetGenerator, self).__init__()

    @execute_once
    def create_valid_raster_product_asset(self):
        asset_dir = os.path.join('RasterProducts', 'Valid')

        self.valid_raster_product_asset = RasterProductAsset(
            os.path.join(asset_dir, self.get_asset_basename(0, '.kiasset')))
        self.valid_raster_product_asset_version = RasterProductAssetVersion(
            self.valid_raster_product_asset, 777)

        out_file = \
            '/opt/google/share/tutorials/fusion/Imagery/bluemarble_4km.tif'
        source = AssetSource(
                uri='khfile://opt/fusion/Imagery/bluemarble_4km.tif',
                size=23866534, mtime=1490904554)

        source_asset = RasterProductSourceAsset(self.valid_raster_product_asset)
        source_asset.add_source(source)
        source_asset_version = RasterProductSourceAssetVersion(source_asset, 778)
        source_asset_version.add_source(source)
        source_asset_version.add_out_file(out_file)
        self.valid_raster_product_asset_version.add_input(source_asset)
        combined_rp_asset = RasterProductCombinedRpAsset(
            self.valid_raster_product_asset)
        combined_rp_asset_version = RasterProductCombinedRpAssetVersion(
            combined_rp_asset, 779)
        combined_rp_asset_version.add_out_file(out_file)
        combined_rp_asset_version.add_source(source)
        self.valid_raster_product_asset_version.add_child(
            combined_rp_asset_version)

    @execute_once
    def generate_valid_raster_projects(self):
        asset_dir = os.path.join('RasterProjects', 'Valid')

        for n in xrange(self.valid_raster_project_count):
            raster_project_asset = RasterProjectAsset(
                os.path.join(
                    asset_dir, self.get_asset_basename(n, '.kiproject')))
            raster_project_asset.add_inset(self.valid_raster_product_asset)
            raster_project_asset_version = RasterProjectAssetVersion(
                raster_project_asset, 888)
            inset = ImageInset(self.valid_raster_product_asset)
            raster_project_asset_version.add_inset(
                inset, self.valid_raster_product_asset_version)

            self.valid_raster_product_asset_version.add_listener(
                raster_project_asset_version)

            raster_project_asset.output(self.asset_root, self.xml_writer)

            self.progress_meter.advance(1)

class TestInvalidAssetGenerator(TestValidAssetGenerator):
    def __init__(self):
        super(TestInvalidAssetGenerator, self).__init__()
        self.invalid_raster_project_count = 1

    @execute_once
    def create_invalid_raster_product_asset(self):
        self.create_valid_raster_product_asset()

        asset_dir = os.path.join('RasterProducts', 'Invalid')

        self.invalid_raster_product_asset = RasterProductAsset(
            os.path.join(asset_dir, 'asset-01-no-content-files.kiasset'))
        self.invalid_raster_product_asset_version = RasterProductAssetVersion(
            self.invalid_raster_product_asset, 666)

        source_asset = RasterProductSourceAsset(
            self.invalid_raster_product_asset)
        _ = RasterProductSourceAssetVersion(source_asset, 667)
        combined_rp_asset = RasterProductCombinedRpAsset(
            self.valid_raster_product_asset)
        _ = RasterProductCombinedRpAssetVersion(combined_rp_asset, 668)

    @execute_once
    def generate_invalid_raster_projects(self):
        asset_dir = os.path.join('RasterProjects', 'Invalid')

        raster_project_asset = RasterProjectAsset(
            os.path.join(
                asset_dir, 'asset-01-input-with-no-content-files.kiproject'))
        raster_project_asset.add_inset(self.invalid_raster_product_asset)
        raster_project_asset_version = RasterProjectAssetVersion(
            raster_project_asset, 999)
        inset = ImageInset(self.invalid_raster_product_asset, '', '')
        inset.data_asset_name = ''
        raster_project_asset_version.add_inset(
            inset, self.invalid_raster_product_asset_version)
        raster_project_asset.output(self.asset_root, self.xml_writer)

        self.progress_meter.advance(1)

class TestAssetGenerator(TestInvalidAssetGenerator):
    def __init__(
        self, output_valid_raster_projects=True,
        output_invalid_raster_projects=True
    ):
        super(TestAssetGenerator, self).__init__()
        self.output_valid_raster_projects = output_valid_raster_projects
        self.output_invalid_raster_projects = output_invalid_raster_projects

    def create_common_assets(self):
        self.create_valid_raster_product_asset()
        self.create_invalid_raster_product_asset()

    def write_common_assets(self):
        self.valid_raster_product_asset.output(self.asset_root, self.xml_writer)
        self.invalid_raster_product_asset.output(
            self.asset_root, self.xml_writer)

        self.progress_meter.advance(2)

    def generate_raster_projects(self):
        if self.output_valid_raster_projects:
            self.generate_valid_raster_projects()
        if self.output_invalid_raster_projects:
            self.generate_invalid_raster_projects()

    def generate_assets(self):
        if self.output_valid_raster_projects:
            self.progress_meter.end_value += self.valid_raster_project_count
        if self.output_invalid_raster_projects:
            self.progress_meter.end_value += self.invalid_raster_project_count

        self.create_common_assets()
        self.generate_raster_projects()
        self.write_common_assets()

        self.progress_meter.close()


def main():
    parser = argparse.ArgumentParser(
        description="Generate Fusion asset XMLs for testing.")

    parser.add_argument(
        '--skip-valid-raster-projects', action='store_true',
        help="Don't generate valid raster project XMLs.")
    parser.add_argument(
        '--skip-invalid-raster-projects', action='store_true',
        help="Don't generate invalid raster project XMLs.")

    args = parser.parse_args()

    generator = TestAssetGenerator(
        not args.skip_valid_raster_projects,
        not args.skip_invalid_raster_projects)
    generator.generate_assets()


if __name__ == '__main__':
    main()
