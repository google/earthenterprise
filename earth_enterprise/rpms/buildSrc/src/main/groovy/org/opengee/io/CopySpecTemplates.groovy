package org.opengee.io

class CopySpecTemplates {
    static def stringStripSuffix(value, suffix) {
        return value.substring(0, value.length() - suffix.length())
    }

    // You can use this function to expand template files in a CopySpec.
    // It's similar to combining `filesMatching()` and `expand()`, but it uses
    // `groovy.text.StreamingTemplateEngine` and defines a few service variables
    // that can be used from inside templates:
    //
    //     'expandTemplateFile': You can use this function from within a template
    // to expand another template file.  E.g.:
    //
    //     # Code inside template file to expand another template file:
    //     <%= expandTemplateFile.call('gevars.sh.template') %>
    //
    //     'thisTemplateFile': A `java.io.File` object corresponding to the 
    // template file being expanded.  This is currently not updated when expanding
    // a template file from a template file, and should probably not be used there.
    //
    // Example:
    //
    //     import org.opengee.io.CopySpecTemplates
    //
    //     task openGeeSharedFiles(type: Copy) {
    //         from file('shared/src')
    //         into file('build/shared')
    //
    //         // Expand all files whose names end in '.template' as templates,
    //         // substituting the 'openGeeVersion' variable in each template:
    //         CopySpecTemplates.expand(delegate,
    //             [
    //                 'openGeeVersion': ospackage.version
    //             ])
    //     }
    static def expand(copySpec, templateVariablesMap, templateSuffix = '.template') {
        def templateVariables = templateVariablesMap.clone()

        def addExpandTemplateFileFunction = !templateVariables.containsKey('expandTemplateFile')
        def addThisTemplateFile = !templateVariables.containsKey('thisTemplateFile')

        copySpec.eachFile {
            if (it.sourceName.endsWith(templateSuffix)) {
                it.name = stringStripSuffix(it.getSourceName(), templateSuffix)

                def templateDir = it.file.parent

                // Add a function for expanding a template from within a template
                // file, if necessary:
                if (addExpandTemplateFileFunction) {
                    def expandTemplate = { text_or_file ->
                        def engine = new groovy.text.SimpleTemplateEngine()
                        def template = engine.createTemplate(text_or_file).make(templateVariables)

                        return template.toString()
                    }

                    templateVariables['expandTemplateFile'] =
                        { template_path ->
                            def path = new File(template_path)

                            expandTemplate(
                                path.absolute ?
                                    path : new File(new File(templateDir), template_path))
                        }
                }

                if (addThisTemplateFile) {
                    templateVariables['thisTemplateFile'] = it.file
                }

                it.filter(
                    org.opengee.io.StreamingTemplateReader,
                    bindings: templateVariables
                )
            }
        }
    }

}
