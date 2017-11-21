package com.tsciences.shell

class TstCommandLine {
    // Returns the standard output that results from executing a given command.
    //  To pipe to standard input set `action` to a lambda that accepts the
    // standard input writer as an argument.
    static def expand(command, error_message_header, action = null) {
        def process = command.execute()
        def stdoutBuffer = new StringBuilder()
        def stderrBuffer = new StringBuilder()

        if (action != null) {
            process.withWriter action
        }
        process.waitForProcessOutput(stdoutBuffer, stderrBuffer)
        def exitCode = process.waitFor()
        if (exitCode != 0) {
            throw new IllegalArgumentException("""\
                ${error_message_header}
                Command: ${command}
                Exit code: ${exitCode}.
                Error output:
                ${stderrBuffer}
                Standard output:
                ${stdoutBuffer}""".stripIndent())
        }

        return stdoutBuffer.toString()
    }

    // Returns the path to a given command found in a standard system PATH:
    static def resolveCommandPath(command_name) {
        return expand(
                ["bash", "-c", "type -P -p ${command_name}"],
                "Failed to find command ${command_name} in current PATH!"
            ).readLines()[0]
    }
}
