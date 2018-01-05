import java.util.regex.Matcher
import java.util.regex.Pattern
import org.gradle.api.tasks.TaskAction
import com.netflix.gradle.plugins.rpm.Rpm
import org.opengee.shell.GeeCommandLine
import org.opengee.os.package.Dependency


class GeeRpm extends com.netflix.gradle.plugins.rpm.Rpm {
    // Gets the name of the RPM package that provides a given capability:
    static def whatProvides(capability_path) {
        return GeeCommandLine.expand(
                ["rpm", "-q", "--queryformat=%{NAME}\\n", "--whatprovides",
                    capability_path],
                "Failed to find an RPM that provides ${capability_path}!"
            ).readLines()[0]
    }

    // Gets the name of the RPM package that provides a given shell command:
    static def whatProvidesCommand(String command_name) {
        return whatProvides(GeeCommandLine.resolveCommandPath(command_name))
    }

    // Returns the name of a Yum package that provides a given command.
    // (E.g., whatProvidesCommand(['sed'] as List) == ['sed'])
    static def whatProvidesCommand(Iterable<String> command_names) {
        return command_names.collect { whatProvidesCommand(it) }
    }

    // Runs `find-provides` from `rpm-build` to find a list of capabilities
    // provided by an RPM that contains a given list of files.
    static def findProvides(Iterable<File> inputFileList) {
        return GeeCommandLine.expand(
                ["/usr/lib/rpm/find-provides"],
                "Runing /usr/lib/rpm/find-provides failed!",
                { stdin -> inputFileList.each { stdin << it << "\n" } }
            ).readLines()
    }

    // Runs `find-requires` from `rpm-build` to find a list of capabilities
    // required by an RPM that contains a given list of files.
    static def findRequires(Iterable<File> inputFileList) {
        return GeeCommandLine.expand(
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
    //     The lambda should map:
    //         Dependency -> Dependency
    //     If the result is `null`, that depedency is not included in the
    // output package.
    Closure autoFindRequiresFilter = { it }

    // A set of `Requires(Pre)` (pre-install) dependencies for the RPM:
    Set<Dependency> requiresPre_set = new HashSet<Dependency>();


    GeeRpm() {
        super()
    }

    def rebuildWithRequiresPre(
        File rpmFile, Iterable<Dependency> requiresPre,
        File buildDir, File outputFile=null
    ) {
        // Write requires filter:
        def requiresFilterFile = new File(buildDir, "requires-filter.sh")

        requiresFilterFile.parentFile.mkdirs()
        requiresFilterFile.write(
"""#! /bin/bash

# Pass through existing requires:
cat

# Add Requires(Pre) statements:
cat <<RequiresEND
${
    requiresPre.collect { "Requires(pre): ${it}" }.join("\n")
 }
RequiresEND
""")

        GeeCommandLine.expand(
            [
                "/usr/bin/rpmrebuild", "--package",
                "--change-spec-requires=/bin/bash ${requiresFilterFile}",
                "--directory=${buildDir}",
                "${rpmFile}"
            ],
            "Rebuilding RPM to add Requires(pre) failed!",
            // Control environment variables. (Needed in case we're running
            // in a minimalistic enviroment like a `scons` target.)
            [
                "PATH=/bin:/usr/bin:/sbin:/usr/sbin",
                "HOME=${buildDir}"
            ]
        )

        if (outputFile != null) {
            java.nio.file.Files.copy(
                new File(buildDir, "${archString}/${rpmFile.name}").toPath(),
                outputFile.toPath(),
                java.nio.file.StandardCopyOption.REPLACE_EXISTING,
                java.nio.file.StandardCopyOption.COPY_ATTRIBUTES );
        }
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

        if (requiresPre_set) {
            def rpmFile =
                new File("${project.buildDir}/distributions/${assembleArchiveName()}")

            rebuildWithRequiresPre(
                rpmFile, requiresPre_set,
                new File("${project.buildDir}/rpmRebuild"), rpmFile)
        }
    }

    // Adds the packages that provide all of the given commands to the package
    // dependency list.
    def requiresCommands(Iterable<String> commands) {
        (commands as Set).each {
            requires(GeeCommandLine.resolveCommandPath(it))
        }
    }

    def requiresPre(Dependency requirement) {
        requiresPre_set.add(requirement)
    }

    def requiresPre(String name, String version, int flags) {
        requiresPre(new Dependency(name, version, flags))
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
            collect { Dependency.fromSpecifier(it) }.
            collect(autoFindProvidesFilter).
            findAll { it != null }.
            each {
                if (it.flags != 0) {
                    provides(it.name, it.version, it.flags)
                } else {
                    provides(it.name)
                }
                autoFoundProvidedCapabilities.add(it.name)
            }
    }

    // Runs `find-requires`, and adds required artifacts to the package.
    //     You probably don't want to call this method directly, but just want
    // to set `autoFindRequires = true`.
    def findAndAddRequires() {
        findRequires().
            collect { Dependency.fromSpecifier(it) }.
            findAll {
                !autoFoundProvidedCapabilities.contains(it.name)
            }.
            collect(autoFindRequiresFilter).
            findAll { it != null }.
            each {
                if (it.flags != 0) {
                    requires(it.name, it.version, it.flags)
                } else {
                    requires(it.name)
                }
            }
    }
}
