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
}