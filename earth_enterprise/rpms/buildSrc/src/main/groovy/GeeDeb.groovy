// Copyright 2017, 2018 the Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import com.netflix.gradle.plugins.deb.Deb
import org.opengee.shell.GeeCommandLine

class GeeDeb extends com.netflix.gradle.plugins.deb.Deb {
    // Get the name of the Deb package that provides a given file path:
    static def whatProvidesFile(String file_path) {
        def commandOutput = GeeCommandLine.expand(
                ["dpkg", "-S", file_path],
                "Failed to find a Deb that provides ${file_path}!"
            )

        return commandOutput.substring(0, commandOutput.indexOf(':'))
    }

    static def whatProvidesCommand(String command_name) {
        return whatProvidesFile(GeeCommandLine.resolveCommandPath(
            command_name))
    }

    static def whatProvidesCommand(Iterable<String> command_name) {
        return command_name.collect { whatProvidesCommand(it) }
    }

    static def findRequires(File[] inputFileList) {
        def libs = new HashSet()

        inputFileList.collect { inputFile ->
            return GeeCommandLine.expand(
                ["ldd", inputFile.toString()],
                "Failed to list shared library dependencies of ${inputFile}!",
                null,
                // Ignore non-binary files:
                [new GeeCommandLine.ExpectedResult(
                    1, "\tnot a dynamic executable\n", "")]
            )
        }.
        findAll { it != null }.
        each { stdOutput ->
            stdOutput.readLines().
            findAll { it.startsWith("  NEEDED ") }.
            collect { it.substring("  NEEDED ".length()).trim() }
        }
    }


    protected File[] packageInputFiles = null

    GeeDeb() {
        super()
    }

    protected void collectPackageInputFilesList() {
        if (packageInputFiles == null) {
            // Get a list of files going into the RPM:
            def filesList = []

            eachFile { filesList << it.file }

            packageInputFiles = filesList as File[]
            println "packageInputFiles: ${packageInputFiles}"
        }
    }

    // Adds the packages that provide all of the given commands to the package
    // dependency list.
    def requireCommands(Iterable<String> commands) {
        (whatProvidesCommand(commands) as Set).each {
            requires(it)
        }
    }

    // Runs find-provides, and add provided artifacts to the package.
    def findAndAddProvides() {
        collectPackageInputFilesList()

        findProvides(packageInputFiles).each { provides(it) }
    }

    // Runs find-requires, and add required packages.
    def findAndAddRequires() {
        collectPackageInputFilesList()

        findRequires(packageInputFiles).each { requires(it) }
    }

    // Formats a multi-line description to be compatible with a Debian build
    void formatPackageDescription(String descrip){
        def formattedDescription = ''
        descrip.eachLine{ line ->
            if (line.trim() == '') {
                if (formattedDescription != '') {
                    formattedDescription += '\n .'
                }
            } else {
                if (formattedDescription == '') {
                    formattedDescription += line.trim()
                } else {
                    formattedDescription += '\n ' + line.trim()
                }
            }
        }

        packageDescription = formattedDescription
    }
}
