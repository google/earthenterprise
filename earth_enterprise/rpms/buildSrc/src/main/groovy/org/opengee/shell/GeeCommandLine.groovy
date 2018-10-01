package org.opengee.shell


class GeeCommandLine {
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
    // accepts the standard input writer as a parameter.
    //     To suppress known command results (return `null`), use the
    // `suppressedResults` parameter.
    static def expand(
        command, String error_message_header,
        Iterable<String> envp = null, File work_dir = null, stdInAction = null,
        Iterable<ExpectedResult> suppressedResults = []
    ) {
        def process = command.execute(envp, work_dir)
        def stdoutBuffer = new StringBuilder()
        def stderrBuffer = new StringBuilder()

        if (stdInAction != null) {
            process.withWriter stdInAction
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
            throw new Exception("""\
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

    static def expand(
        command, String error_message_header, stdInAction,
        Iterable<ExpectedResult> suppressedResults = []
    ) {
        return expand(command, error_message_header, null, null, stdInAction,
            suppressedResults)
    }

    // Returns the path to a given command found in a standard system PATH:
    static def resolveCommandPath(command_name) {
        return expand(
                ["bash", "-l", "-c", "type -P -p ${command_name}"],
                "Failed to find command ${command_name} in current PATH!"
            ).readLines()[0]
    }
}
