import java.util.regex.Matcher
import java.util.regex.Pattern
import org.gradle.api.tasks.TaskAction
import com.netflix.gradle.plugins.rpm.Rpm
import com.tsciences.shell.TstCommandLine

class TstRpm extends com.netflix.gradle.plugins.rpm.Rpm {
    // A class that stores information about a dependency.
    // (E.g., 'perl >= 1:5')
    @groovy.transform.InheritConstructors
    class Dependency {
        String capabilityName
        String version = null

        // Values from org.freecompany.redline.header.Flags (like 'EQUAL' and
        // 'GREATER') bit-wise OR'd:
        int versionComparisonFlag = 0

        Dependency(String dependencySpecifier) {
            Matcher m = Pattern.compile('[<=>]+').matcher(dependencySpecifier)

            // Parse strings like 'name(arch) <=> 1.0+3':
            if (m.find()) {
                capabilityName = dependencySpecifier.substring(0, m.start()).trim()
                m.group().each {
                    switch (it) {
                        case '<':
                            versionComparisonFlag |=
                                org.freecompany.redline.header.Flags.LESS
                            break
                        case '=':
                            versionComparisonFlag |=
                                org.freecompany.redline.header.Flags.EQUAL
                            break
                        case '>':
                            versionComparisonFlag |=
                                org.freecompany.redline.header.Flags.GREATER
                            break
                    }
                }
                version = dependencySpecifier.substring(m.end()).trim()
            } else {
                capabilityName = dependencySpecifier
            }
        }
    }

    // Gets the name of the RPM package that provides a given capability:
    static def whatProvides(capability_path) {
        return TstCommandLine.expand(
                ["rpm", "-q", "--queryformat=%{NAME}\\n", "--whatprovides",
                    capability_path],
                "Failed to find an RPM that provides ${capability_path}!"
            ).readLines()[0]
    }

    // Gets the name of the RPM package that provides a given shell command:
    static def whatProvidesCommand(String command_name) {
        return whatProvides(TstCommandLine.resolveCommandPath(command_name))
    }

    // Returns the name of a Yum package that provides a given command.
    // (E.g., whatProvidesCommand(['sed'] as List) == ['sed'])
    static def whatProvidesCommand(Iterable<String> command_names) {
        return command_names.collect { whatProvidesCommand(it) }
    }

    // Runs `find-provides` from `rpm-build` to find a list of capabilities
    // provided by an RPM that contains a given list of files.
    static def findProvides(Iterable<File> inputFileList) {
        return TstCommandLine.expand(
                ["/usr/lib/rpm/find-provides"],
                "Runing /usr/lib/rpm/find-provides failed!",
                { stdin -> inputFileList.each { stdin << it << "\n" } }
            ).readLines()
    }

    // Runs `find-requires` from `rpm-build` to find a list of capabilities
    // required by an RPM that contains a given list of files.
    static def findRequires(Iterable<File> inputFileList) {
        return TstCommandLine.expand(
                ["/usr/lib/rpm/find-requires"],
                "Runing /usr/lib/rpm/find-provides failed!",
                { stdin -> inputFileList.each { stdin << it << "\n" } }
            ).readLines().
            findAll { !(it.trim() in ['', '#', 'from']) }
    }



    // Whether to use `find-provides` from `rpm-build` to automatically find
    // capabilities provided by this RPM:
    boolean autoFindProvides = false

    // A lambda executed on each automatically found provided capability.
    //   The lambda should map:
    //       Dependency -> Dependency
    //   If the result is `null`, that depedency is not included in the output
    // package.
    Closure autoFindProvidesFilter = { it }

    protected HashSet<String> autoFoundProvidedCapabilities = null

    // Whether to use `find-requires` from `rpm-build` to automatically find
    // capabilities required by this RPM:
    boolean autoFindRequires = false

    // A lambda executed on each automatically found required capability.
    //   The lambda should map:
    //       Dependency -> Dependency
    //   If the result is `null`, that depedency is not included in the output
    // package.
    Closure autoFindRequiresFilter = { it }


    TstRpm() {
        super()
    }

    // Override the @TaskAction from the base class, so we can run automatic
    // depedency detection, first.
    @Override
    @TaskAction
    protected void copy() {
        if (autoFindProvides) {
            findAndAddProvides()
        }

        if (autoFindRequires) {
            findAndAddRequires()
        }
        super.copy()
    }

    // Adds the packages that provide all of the given commands to the package
    // dependency list.
    def requireCommands(Iterable<String> commands) {
        (commands as Set).each {
            requires(TstCommandLine.resolveCommandPath(it))
        }
    }

    // Returns a list of capability specifications that the files going into
    // this RPM provide.
    def findProvides() {
        return findProvides(source.files)
    }

    // Returns a list of capability specifications that the files going into
    // this RPM require.
    def findRequires() {
        return findRequires(source.files)
    }

    // Runs `find-provides`, and adds provided artifacts to the package.
    //     You probably don't want to call this method directly, but just want
    // to set `autoFindProvides = true`.
    def findAndAddProvides() {
        autoFoundProvidedCapabilities = new HashSet<String>()

        findProvides().
            collect { new Dependency(it) }.
            collect(autoFindProvidesFilter).
            findAll { it != null }.
            each {
                if (it.versionComparisonFlag != 0) {
                    provides(it.capabilityName, it.version,
                        it.versionComparisonFlag)
                } else {
                    provides(it.capabilityName)
                }
                autoFoundProvidedCapabilities.add(it.capabilityName)
            }
    }

    // Runs `find-requires`, and adds required artifacts to the package.
    //     You probably don't want to call this method directly, but just want
    // to set `autoFindRequires = true`.
    def findAndAddRequires() {
        findRequires().
            collect { new Dependency(it) }.
            findAll {
                !autoFoundProvidedCapabilities.contains(it.capabilityName)
            }.
            collect(autoFindRequiresFilter).
            findAll { it != null }.
            each {
                if (it.versionComparisonFlag != 0) {
                    requires(it.capabilityName, it.version,
                        it.versionComparisonFlag)
                } else {
                    requires(it.capabilityName)
                }
            }
    }
}
