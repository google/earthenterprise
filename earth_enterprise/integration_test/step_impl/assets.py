from getgauge.python import step
import os
import re
import subprocess
import time

ASSET_ROOT = "/gevol/assets"
BASE_ASSET_PATH = "gauge_tests"
DATABASE_PATH = os.path.join(BASE_ASSET_PATH, "Databases")
MAP_LAYER_PATH = os.path.join(BASE_ASSET_PATH, "MapLayers")
IMAGERY_PROJECT_PATH = os.path.join(BASE_ASSET_PATH, "Projects", "Imagery")
TERRAIN_PROJECT_PATH = os.path.join(BASE_ASSET_PATH, "Projects", "Terrain")
VECTOR_PROJECT_PATH = os.path.join(BASE_ASSET_PATH, "Projects", "Vector")
MAP_PROJECT_PATH = os.path.join(BASE_ASSET_PATH, "Projects", "Map")
IMAGERY_RESOURCE_PATH = os.path.join(BASE_ASSET_PATH, "Resources", "Imagery")
TERRAIN_RESOURCE_PATH = os.path.join(BASE_ASSET_PATH, "Resources", "Terrain")
VECTOR_RESOURCE_PATH = os.path.join(BASE_ASSET_PATH, "Resources", "Vector")

def get_env_value(sKey):
   return os.getenv(sKey, "unset")

def get_asset_root():
  return os.getenv('ge_asset_root')

def get_src_data_path():
  return os.getenv('ge_src_data_path')

def call(command, errorMsg):
  result = subprocess.call(command)
  assert result == 0, errorMsg

def make_list(data):
  if not isinstance(data, list):
    data = [data]
  return data

def list_from_table(table):
  return [row[0] for row in table]

def get_status(asset):
  return subprocess.check_output(["/opt/google/bin/gequery", "--status", asset]).strip()

def do_create_imagery_proj(project, isMercator):
  commandLine = ["/opt/google/bin/genewimageryproject", "-o", os.path.join(IMAGERY_PROJECT_PATH, project)]
  if isMercator:
    commandLine.append("--mercator")
  call(commandLine, "Failed to create imagery project %s" % project)

@step("Create imagery project <project>")
def create_imagery_proj(project):
  do_create_imagery_proj(project, isMercator=False)

@step("Create mercator imagery project <project>")
def create_merc_imagery_proj(project):
  do_create_imagery_proj(project, isMercator=True)

@step("Create terrain project <project>")
def create_terrain_proj(project):
  call(["/opt/google/bin/genewterrainproject", "-o", os.path.join(TERRAIN_PROJECT_PATH, project)],
       "Failed to create terrain project %s" % project)

@step("Create vector project <project>")
def create_vector_proj(project):
  call(["/opt/google/bin/genewvectorproject", "-o", os.path.join(VECTOR_PROJECT_PATH, project)],
       "Failed to create vector project %s" % project)

def do_create_imagery_resource(resource, srcPath, isMercator):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  commandLine = ["/opt/google/bin/genewimageryresource", "-o", os.path.join(IMAGERY_RESOURCE_PATH, resource), "%s" % srcPath]
  if isMercator:
    commandLine.append("--mercator")
  call(commandLine, "Failed to create imagery resource %s from %s" % (resource, srcPath))

@step("Create imagery resource <resource> from <srcPath>")
def create_imagery_resource(resource, srcPath):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  do_create_imagery_resource(resource, srcPath, isMercator=False)

@step("Create mercator imagery resource <resource> from <srcPath>")
def create_merc_imagery_resource(resource, srcPath):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  do_create_imagery_resource(resource, srcPath, isMercator=True)

def do_add_imagery_resource_to_project(resource, project, isMercator):
  commandLine = ["/opt/google/bin/geaddtoimageryproject", "-o", os.path.join(IMAGERY_PROJECT_PATH, project), os.path.join(IMAGERY_RESOURCE_PATH, resource)]
  if isMercator:
    commandLine.append("--mercator")
  call(commandLine, "Failed to add imagery resource %s to project %s" % (resource, project))

@step("Add imagery resource <resource> to project <project>")
def add_imagery_resource_to_project(resource, project):
  do_add_imagery_resource_to_project(resource, project, isMercator=False)

@step("Drop imagery resource <resource> from project <project>")
def drop_imagery_resource_to_project(resource, project):
  commandLine = ["/opt/google/bin/gedropfromimageryproject", "-o", os.path.join(IMAGERY_PROJECT_PATH, project), os.path.join(IMAGERY_RESOURCE_PATH, resource)]
  call(commandLine, "Failed to drop imagery resource %s from project %s" % (resource, project))

@step("Add mercator imagery resource <resource> to project <project>")
def add_merc_imagery_resource_to_project(resource, project):
  do_add_imagery_resource_to_project(resource, project, isMercator=True)

@step("Create imagery resource <resource> from <srcPath> and add to project <project>")
def create_imagery_resource_add_to_proj(resource, srcPath, project):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  create_imagery_resource(resource, srcPath)
  add_imagery_resource_to_project(resource, project)

@step("Create terrain resource <resource> from <srcPath>")
def create_terrain_resource(resource, srcPath):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  call(["/opt/google/bin/genewterrainresource", "-o", os.path.join(TERRAIN_RESOURCE_PATH, resource), "%s" % srcPath],
       "Failed to create terrain resource %s from %s" % (resource, srcPath))

@step("Add terrain resource <resource> to project <project>")
def add_terrain_resource_to_project(resource, project):
  call(["/opt/google/bin/geaddtoterrainproject", "-o", os.path.join(TERRAIN_PROJECT_PATH, project), os.path.join(TERRAIN_RESOURCE_PATH, resource)],
       "Failed to add terrain resource %s to project %s" % (resource, project))

@step("Create terrain resource <resource> from <srcPath> and add to project <project>")
def create_terrain_resource_add_to_proj(resource, srcPath, project):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  create_terrain_resource(resource, srcPath)
  add_terrain_resource_to_project(resource, project)

@step("Create vector resource <resource> from <srcPath>")
def create_vector_resource(resource, srcPath):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  call(["/opt/google/bin/genewvectorresource", "-o", os.path.join(VECTOR_RESOURCE_PATH, resource), "%s" % srcPath],
       "Failed to create vector resource %s from %s" % (resource, srcPath))

@step("Add vector resource <resource> to project <project>")
def add_vector_resource_to_project(resource, project):
  call(["/opt/google/bin/geaddtovectorproject", "-o", os.path.join(VECTOR_PROJECT_PATH, project),
        "--template", os.path.join(get_src_data_path(), "CA_POIs_template.khdsp"),
        os.path.join(VECTOR_RESOURCE_PATH, resource)],
       "Failed to add vector resource %s to project %s" % (resource, project))

@step("Create and build vector resource <resource> from <srcPath> and add to project <project>")
def create_vector_resource_add_to_proj(resource, srcPath, project):
  srcPath = os.path.join(get_src_data_path(), srcPath)
  create_vector_resource(resource, srcPath)
  # Vector resources must be built before they can be added to projects
  build_vector_resource(resource)
  wait_for_vector_resource_state(resource, "Succeeded")
  verify_state_vector_resource(resource, "Succeeded")
  add_vector_resource_to_project(resource, project)

@step("Verify that imagery project <project> has no versions")
def verify_imagery_proj_no_versions(project):
  output = subprocess.check_output(["/opt/google/bin/gequery", "--versions", os.path.join(IMAGERY_PROJECT_PATH, project)])
  assert output.strip() == "NO VERSIONS", "Imagery project %s has more than 0 versions. Output: %s" % (project, output)

def asset_operation(operation, asset, assetType):
  call(["/opt/google/bin/ge%s" % operation, asset],
       "Failed to %s %s %s" % (operation, assetType, asset))

def imagery_project_operation(operation, project):
  asset_operation(operation, os.path.join(IMAGERY_PROJECT_PATH, project), "imagery project")

@step("Build imagery project <project>")
def build_imagery_project(project):
  imagery_project_operation("build", project)

@step("Cancel imagery project <project>")
def cancel_imagery_project(project):
  imagery_project_operation("cancel", project)

@step("Resume imagery project <project>")
def resume_imagery_project(project):
  imagery_project_operation("resume", project)

@step("Clean imagery project <project>")
def clean_imagery_project(project):
  imagery_project_operation("clean", project)

def imagery_resource_operation(operation, resource):
  asset_operation(operation, os.path.join(IMAGERY_RESOURCE_PATH, resource), "imagery resource")

@step("Cancel imagery resource <resource>")
def cancel_imagery_resource(resource):
  imagery_resource_operation("cancel", resource)

@step("Resume imagery resource <resource>")
def resume_imagery_resource(resource):
  imagery_resource_operation("resume", resource)

@step("Clean imagery resource <resource>")
def clean_imagery_resource(resource):
  imagery_resource_operation("clean", resource)

@step("Build imagery resource <resource>")
def build_imagery_resource(resource):
  imagery_resource_operation("build", resource)

def vector_resource_operation(operation, resource):
  asset_operation(operation, os.path.join(VECTOR_RESOURCE_PATH, resource), "vector resource")

@step("Build vector resource <resource>")
def build_vector_resource(resource):
  vector_resource_operation("build", resource)

def database_operation(operation, database):
  asset_operation(operation, os.path.join(DATABASE_PATH, database), "database")

@step("Build database <database>")
def build_database(database):
  database_operation("build", database)

@step("Cancel database <database>")
def cancel_database(database):
  database_operation("cancel", database)

@step("Clean database <database>")
def clean_database(database):
  database_operation("clean", database)

def map_project_operation(operation, project):
  asset_operation(operation, os.path.join(MAP_PROJECT_PATH, project), "map project")

@step("Cancel map project <project>")
def cancel_map_project(project):
  map_project_operation("cancel", project)

def get_short_project_name(project):
  prefix = "StatePropagationTest_"
  if project.startswith(prefix):
    return project[len(prefix):]
  else:
    return project

@step("Create and build default project <project>")
def create_and_build_blue_marble_proj(project):
  shortName = get_short_project_name(project)
  create_imagery_proj(project)
  create_imagery_resource_add_to_proj("BlueMarble_" + shortName, os.path.join(get_src_data_path(), "Imagery/bluemarble_4km.tif"), project)
  create_imagery_resource_add_to_proj("i3SF15meter_" + shortName,  os.path.join(get_src_data_path(), "Imagery/i3SF15-meter.tif"), project)
  create_imagery_resource_add_to_proj("USGSLanSat_" + shortName,  os.path.join(get_src_data_path(), "Imagery/usgsLanSat.tif"), project)
  create_imagery_resource_add_to_proj("SFHiRes_" + shortName,  os.path.join(get_src_data_path(), "Imagery/usgsSFHiRes.tif"), project)
  verify_imagery_proj_no_versions(project)
  build_imagery_project(project)

def wait_until_state(asset, statuses):
  statuses = make_list(statuses)
  while True:
    status = get_status(asset)
    if status in statuses:
      break
    # Check for terminal states
    assert status not in ["Blocked", "Failed", "Canceled", "Offline", "Bad", "Succeeded"], \
        "Asset %s entered terminal state %s while waiting for state %s" % (asset, status, " ".join(statuses))
    time.sleep(1)

@step("Wait for imagery project <project> to reach state <state>")
def wait_for_imagery_project_state(project, state):
  wait_until_state(os.path.join(IMAGERY_PROJECT_PATH, project), state)
  verify_state_imagery_proj(project, state)

@step("Wait for vector resource <resource> to reach state <state>")
def wait_for_vector_resource_state(resource, state):
  wait_until_state(os.path.join(VECTOR_RESOURCE_PATH, resource), state)
  verify_state_vector_resource(resource, state)

@step("Wait for database <database> to reach state <state>")
def wait_for_database_state(database, state):
  wait_until_state(os.path.join(DATABASE_PATH, database), state)
  verify_state_database(database, state)

@step("Wait for mercator database <database> to reach state <state>")
def wait_for_mercator_database_state(database, state):
  wait_until_state(os.path.join(DATABASE_PATH, database), state)
  verify_state_mercator_database(database, state)

def verify_state(path, states):
  states = make_list(states)
  status = get_status(path)
  assert status in states, "Asset %s in unexpected state. Expected: %s, actual: %s" % (path, " ".join(states), status)

def get_one_input(path, project, extension):
  hasInput = re.compile(r".*/(\w*\." + extension + ").*")
  deps = subprocess.check_output(["/opt/google/bin/gequery", "--dependencies", os.path.join(path, project)])
  depsIter = iter(deps.splitlines())
  for line in depsIter:
    match = hasInput.match(line)
    if match:
      return match.group(1)
  assert False, "Could not find input for " + project

def get_one_packlevel(path, resource, ext):
  fullPath = os.path.join(ASSET_ROOT, path, resource, "CombinedRP." + ext, "packgen." + ext)
  files = os.listdir(fullPath)
  for f in files:
    if f.startswith("packlevel"):
      return f
  assert False, "Could not find packlevel for " + resource

def verify_state_imagery_proj_helper(project, state, projExt, resExt):
  verify_state(os.path.join(IMAGERY_PROJECT_PATH, project), state)
  if state == "Succeeded":
    # Check the states of the project's children
    verify_state(os.path.join(IMAGERY_PROJECT_PATH, project + "." + projExt, "layerjs"), state)
    verify_state(os.path.join(IMAGERY_PROJECT_PATH, project + "." + projExt, "dbroot"), state)
    verify_state(os.path.join(IMAGERY_PROJECT_PATH, project + "." + projExt, "geindex"), state)
    # Check the tasks that are built under the resource but only when the project is built (only check one resource)
    resource = get_one_input(IMAGERY_PROJECT_PATH, project, resExt)
    # In one test the CombinedRP ends up in a "cleaned" state even though the packgens
    # succeeded. I'm not sure why that happens, but if it's a bug it's been there for a while.
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource, "CombinedRP"), ["Succeeded", "Cleaned"])
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource, "CombinedRP.kia", "packgen"), state)
    # Only check one pack level
    packlev = get_one_packlevel(IMAGERY_RESOURCE_PATH, resource, "kia")
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource, "CombinedRP.kia", "packgen.kia", packlev), state)

@step("Verify that the state of imagery project <project> is <state>")
def verify_state_imagery_proj(project, state):
  verify_state_imagery_proj_helper(project, state, "kiproject", "kiasset")

@step("Verify that the state of mercator imagery project <project> is <state>")
def verify_state_mercator_imagery_proj(project, state):
  verify_state_imagery_proj_helper(project, state, "kimproject", "kimasset")

@step("Verify that the state of imagery project <project> is in <table>")
def verify_state_imagery_project_table(project, table):
  states = list_from_table(table)
  verify_state(os.path.join(IMAGERY_PROJECT_PATH, project), states)

def verify_state_imagery_resource_helper(resource, state, resExt):
  verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource), state)
  if state == "Succeeded":
    # Check the states of the resource's children
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource + "." + resExt, "maskgen"), state)
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource + "." + resExt, "maskproduct"), state)
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource + "." + resExt, "product"), state)
    verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource + "." + resExt, "source"), state)

@step("Verify that the state of imagery resource <resource> is <state>")
def verify_state_imagery_resource(resource, state):
  verify_state_imagery_resource_helper(resource, state, "kiasset")

@step("Verify that the state of mercator imagery resource <resource> is <state>")
def verify_state_mercator_imagery_resource(resource, state):
  verify_state_imagery_resource_helper(resource, state, "kimasset")

@step("Verify that the state of imagery resource <resource> is in <table>")
def verify_state_imagery_resource_table(resource, table):
  states = list_from_table(table)
  verify_state(os.path.join(IMAGERY_RESOURCE_PATH, resource), states)

@step("Verify that the state of images for default project <project> is <state>")
def verify_state_default_images(project, state):
  shortName = get_short_project_name(project)
  verify_state_imagery_resource("BlueMarble_" + shortName, state)
  verify_state_imagery_resource("i3SF15meter_" + shortName, state)
  verify_state_imagery_resource("USGSLanSat_" + shortName, state)
  verify_state_imagery_resource("SFHiRes_" + shortName, state)

@step("Verify that the state of images for default project <project> is in <table>")
def verify_state_default_images_table(project, table):
  states = list_from_table(table)
  verify_state_default_images(project, states)

@step("Verify that the state of terrain project <project> is <state>")
def verify_state_terrain_proj(project, state):
  verify_state(os.path.join(TERRAIN_PROJECT_PATH, project), state)
  if state == "Succeeded":
    # Check the states of the project's children
    verify_state(os.path.join(TERRAIN_PROJECT_PATH, project + ".ktproject", "dbroot"), state)
    verify_state(os.path.join(TERRAIN_PROJECT_PATH, project + ".ktproject", "geindex"), state)
    # Check the tasks that are built under the resource but only when the project is built (only check one resource)
    resource = get_one_input(TERRAIN_PROJECT_PATH, project, "ktasset")
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource, "CombinedRP"), state)
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource, "CombinedRP.kta", "packgen"), state)
    # Only check one pack level
    packlev = get_one_packlevel(TERRAIN_RESOURCE_PATH, resource, "kta")
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource, "CombinedRP.kta", "packgen.kta", packlev), state)

@step("Verify that the state of terrain project <project> is in <table>")
def verify_state_terrain_project_table(project, table):
  states = list_from_table(table)
  verify_state(os.path.join(TERRAIN_PROJECT_PATH, project), states)

@step("Verify that the state of terrain resource <resource> is <state>")
def verify_state_terrain_resource(resource, state):
  verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource), state)
  if state == "Succeeded":
    # Check the states of the resource's children
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource + ".ktasset", "maskgen"), state)
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource + ".ktasset", "maskproduct"), state)
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource + ".ktasset", "product"), state)
    verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource + ".ktasset", "source"), state)

@step("Verify that the state of terrain resource <resource> is in <table>")
def verify_state_terrain_resource_table(resource, table):
  states = list_from_table(table)
  verify_state(os.path.join(TERRAIN_RESOURCE_PATH, resource), states)

@step("Verify that the state of vector project <project> is <state>")
def verify_state_vector_proj(project, state):
  verify_state(os.path.join(VECTOR_PROJECT_PATH, project), state)
  if state == "Succeeded":
    # Check the states of the project's children
    verify_state(os.path.join(VECTOR_PROJECT_PATH, project + ".kvproject", "dbroot"), state)
    verify_state(os.path.join(VECTOR_PROJECT_PATH, project + ".kvproject", "geindex"), state)
    verify_state(os.path.join(VECTOR_PROJECT_PATH, project + ".kvproject", "layer005"), state)
    verify_state(os.path.join(VECTOR_PROJECT_PATH, project + ".kvproject", "layer005.kva", "layer005packet"), state)
    verify_state(os.path.join(VECTOR_PROJECT_PATH, project + ".kvproject", "layer005.kva", "query"), state)

@step("Verify that the state of vector project <project> is in <table>")
def verify_state_vector_project_table(project, table):
  states = list_from_table(table)
  verify_state(os.path.join(VECTOR_PROJECT_PATH, project), states)

@step("Verify that the state of vector resource <resource> is <state>")
def verify_state_vector_resource(resource, state):
  verify_state(os.path.join(VECTOR_RESOURCE_PATH, resource), state)
  if state == "Succeeded":
    # Check the states of the resource's children
    verify_state(os.path.join(VECTOR_RESOURCE_PATH, resource + ".kvasset", "source"), state)

@step("Verify that the state of vector resource <resource> is in <table>")
def verify_state_vector_resource_table(resource, table):
  states = list_from_table(table)
  verify_state(os.path.join(VECTOR_RESOURCE_PATH, resource), states)

@step("Verify that the state of database <database> is <state>")
def verify_state_database(database, state):
  verify_state(os.path.join(DATABASE_PATH, database), state)
  if state == "Succeeded":
    # Check the states of the database's children
    verify_state(os.path.join(DATABASE_PATH, database + ".kdatabase", "gedb"), state)
    verify_state(os.path.join(DATABASE_PATH, database + ".kdatabase", "unifiedindex"), state)
    verify_state(os.path.join(DATABASE_PATH, database + ".kdatabase", "qtpacket"), state)
    terrainPath = os.path.join(DATABASE_PATH, database + ".kdatabase", "terrain")
    if os.path.isdir(terrainPath + ".kta"):
      # Only check for the terrain task if it's there. Not all databases have terrain.
      verify_state(os.path.join(DATABASE_PATH, database + ".kdatabase", "terrain"), state)

@step("Verify that the state of mercator database <database> is <state>")
def verify_state_mercator_database(database, state):
  verify_state(os.path.join(DATABASE_PATH, database), state)
  if state == "Succeeded":
    # Check the states of the database's children
    verify_state(os.path.join(DATABASE_PATH, database + ".kmmdatabase", "mapdb"), state)
    verify_state(os.path.join(DATABASE_PATH, database + ".kmmdatabase", "unifiedindex"), state)

@step("Verify that the state of map layer <layer> is <state>")
def verify_state_map_layer(layer, state):
  verify_state(os.path.join(MAP_LAYER_PATH, layer), state)

@step("Verify that the state of map layer <layer> is in <table>")
def verify_state_map_layer(layer, table):
  states = list_from_table(table)
  verify_state(os.path.join(MAP_LAYER_PATH, layer), states)
  
@step("Verify that the state of map project <project> is <state>")
def verify_state_map_project(project, state):
  verify_state(os.path.join(MAP_PROJECT_PATH, project), state)

@step("Create database <database> from imagery project <imagery>, terrain project <terrain>, and vector project <vector>")
def create_database(database, imagery, terrain, vector):
  call(["/opt/google/bin/genewdatabase", "-o", os.path.join(DATABASE_PATH, database),
       "--imagery", os.path.join(IMAGERY_PROJECT_PATH, imagery),
       "--terrain", os.path.join(TERRAIN_PROJECT_PATH, terrain),
       "--vector", os.path.join(VECTOR_PROJECT_PATH, vector)],
       "Failed to create database %s" % database)

@step("Create database <database> from imagery project <imagery>")
def create_database_imagery(database, imagery):
  call(["/opt/google/bin/genewdatabase", "-o", os.path.join(DATABASE_PATH, database),
       "--imagery", os.path.join(IMAGERY_PROJECT_PATH, imagery)],
       "Failed to create database %s" % database)

@step("Create map layer <layer> from resource <resource>")
def create_map_layer_from_resource(layer, resource):
  call(["/opt/google/bin/genewmaplayer", "--legend", layer, "--output", os.path.join(MAP_LAYER_PATH, layer),
        "--template", os.path.join(get_src_data_path(), "CA_POIs_template.kmdsp"),
        os.path.join(VECTOR_RESOURCE_PATH, resource)],
       "Failed to create map layer %s" % layer)

@step("Create map project <project> from layer <layer>")
def create_map_project(project, layer):
  call(["/opt/google/bin/genewmapproject", "-o", os.path.join(MAP_PROJECT_PATH, project), os.path.join(MAP_LAYER_PATH, layer)],
       "Failed to create map project %s" % project)

@step("Create map database <database> from imagery project <imagery> and map project <mapProject>")
def create_map_database(database, imagery, mapProject):
  call(["/opt/google/bin/genewmapdatabase", "-o", os.path.join(DATABASE_PATH, database),
        "--imagery", os.path.join(IMAGERY_PROJECT_PATH, imagery),
        "--map", os.path.join(MAP_PROJECT_PATH, mapProject), "--mercator"],
       "Failed to create map database %s" % database)

def mark_bad(asset):
  call(["/opt/google/bin/gesetbad", asset],
       "Failed to mark %s as bad" % asset)

@step("Mark imagery resource <resource> bad")
def mark_imagery_resource_bad(resource):
  mark_bad(os.path.join(IMAGERY_RESOURCE_PATH, resource))

@step("Mark imagery project <project> bad")
def mark_imagery_project_bad(project):
  mark_bad(os.path.join(IMAGERY_PROJECT_PATH, project))

@step("Mark database <database> bad")
def mark_database_bad(database):
  mark_bad(os.path.join(DATABASE_PATH, database))

def mark_good(asset):
  call(["/opt/google/bin/geclearbad", asset],
       "Failed to mark %s as good" % asset)

@step("Mark imagery resource <resource> good")
def mark_imagery_resource_good(resource):
  mark_good(os.path.join(IMAGERY_RESOURCE_PATH, resource))

@step("Mark imagery project <project> good")
def mark_imagery_project_good(project):
  mark_good(os.path.join(IMAGERY_PROJECT_PATH, project))

@step("Mark database <database> good")
def mark_database_good(database):
  mark_good(os.path.join(DATABASE_PATH, database))
