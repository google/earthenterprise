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
            ).readLines().
            findAll { !(it.trim() in ['', '#', 'from']) }
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

    // Runs find-provides and find-requires, and adds provided artifacts to
    // the package.
    def findAndAddProvidesAndRequires() {
        def providedCapabilities = new HashSet<String>()

        findProvides(source.files).each {
            provides(it)
            providedCapabilities.add(it.find(/[^<=>]+/).trim())
        }
        findRequires(source.files).each {
            if (!providedCapabilities.contains(it.find(/[^<=>]+/).trim())) {
                requires(it)
            }
        }
    }


    def autoPackageDependencyInjection() {
        println findRequires(source.files).dump()   
        println findProvides(source.files).dump()
        def reqs = findRequires(source.files)
        def provs = findProvides(source.files)
        reqs.removeAll( provs )

        println "XXXXXXXXXXXXXXXXXXX"
        println 'Source: ' + source
        println '    provides : ' + provs
        println '    reqs     : ' + reqs
      
        println "XXXXXXXXXXXXXXXXXXX"
        reqs.each{ r-> 
            println "Adding req: " + r
            requires( r ) 
        }
        provs.each{ p->
            println "Adding prov: " + p
             provides( p ) 
        }
        
    }
}
