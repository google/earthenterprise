import com.netflix.gradle.plugins.rpm.Rpm
import com.tsciences.shell.TstCommandLine

class TstRpm extends com.netflix.gradle.plugins.rpm.Rpm {
    // Get the name of the RPM package that provides a given capability:
    static def whatProvides(capability_path) {
        return TstCommandLine.expand(
                ["rpm", "-q", "--queryformat=%{NAME}\\n", "--whatprovides",
                    capability_path],
                "Failed to find an RPM that provides ${capability_path}!"
            ).readLines()[0]
    }

    // Get the name of the RPM package that provides a given shell command:
    static def whatProvidesCommand(String command_name) {
        return whatProvides(TstCommandLine.resolveCommandPath(command_name))
    }

    static def whatProvidesCommand(Iterable<String> command_names) {
        return command_names.collect { whatProvidesCommand(it) }
    }

    static def findProvides(Iterable<File> inputFileList) {
        return TstCommandLine.expand(
                ["/usr/lib/rpm/find-provides"],
                "Runing /usr/lib/rpm/find-provides failed!",
                { stdin -> inputFileList.each { stdin << it << "\n" } }
            ).readLines()
    }

    static def findRequires(Iterable<File> inputFileList) {
        return TstCommandLine.expand(
                ["/usr/lib/rpm/find-requires"],
                "Runing /usr/lib/rpm/find-provides failed!",
                { stdin -> inputFileList.each { stdin << it << "\n" } }
            ).readLines().
            findAll { !(it.trim() in ['', '#', 'from']) }
    }

    TstRpm() {
        super()
    }

    // Adds the packages that provide all of the given commands to the package
    // dependency list.
    def requireCommands(Iterable<String> commands) {
        (commands as Set).each {
            requires(TstCommandLine.resolveCommandPath(it))
        }
    }

    // Runs find-provides, and add provided artifacts to the package.
    def findAndAddProvides() {
        findProvides(source.files).each { provides(it) }
    }

    // Runs find-requires, and add required packages.
    def findAndAddRequires() {
        findRequires(source.files).each { requires(it) }
    }
}
