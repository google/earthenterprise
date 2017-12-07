package com.tsciences.shell

class TstCommandLine {
    class ExpectedResult {
        Integer exitCode = null
        String stdOut = null
        String stdErr = null

        ExpectedResult(Integer exitCode, String stdOut, String stdErr) {
            this.exitCode = exitCode
            this.stdOut = stdOut
            this.stdErr = stdErr
        }
    }

    // Returns the standard output that results from executing a given command.
    //     To pipe to standard input set `stdInAction` to a lambda that
    // accepts the standard input writer as an parameter.
    //     To suppress known command results (return `null`), use the
    // `suppressedResults` parameter.
    static def expand(
        command, error_message_header, stdInAction = null,
        Iterable<ExpectedResult> suppressedResults = []
    ) {
        def process = command.execute()
        def stdoutBuffer = new StringBuilder()
        def stderrBuffer = new StringBuilder()

        if (stdInAction != null) {
            process.withWriter action
        }
        process.waitForProcessOutput(stdoutBuffer, stderrBuffer)
        def exitCode = process.waitFor()
        suppressedResults.each {
            if (
                (it.exitCode == null || it.exitCode == exitCode) &&
                (it.stdOut == null || it.stdOut == stdoutBuffer) &&
                (it.stdErr == null || it.stdErr == stderrBuffer)
            ) {
                // Suppress result:
                return null;
            }
        }
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
