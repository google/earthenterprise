package org.opengee.io

import java.io.Reader
import java.io.FilterReader
import java.io.InputStream
import java.io.IOException
import java.io.PipedReader
import java.io.Reader
import java.io.StringReader
import java.io.StringWriter
import org.gradle.api.Transformer



class StreamingTemplateReader extends FilterReader {
    def engine = new groovy.text.StreamingTemplateEngine()
    groovy.text.Template template
    protected def bindingsMap = [:]
    protected String expandedTemplate
    protected Reader expandedTemplateReader = null

    public StreamingTemplateReader(Reader inputReader) {
        super(inputReader)

        // StreamingTemplateEngine only works with readers that support the
        // `mark()` method.
        if (inputReader.markSupported()) {
            template = engine.createTemplate(inputReader)
        } else {
            // So, we read the input into a string. ;P
            char[] readBuffer = new char[8 * 1024];
            StringBuilder builder = new StringBuilder();
            int numCharsRead;
            while (
                (numCharsRead =
                    inputReader.read(readBuffer, 0, readBuffer.length)) > -1
            ) {
                builder.append(readBuffer, 0, numCharsRead);
            }
            inputReader.close();
            String inputTemplate = builder.toString();

            template = engine.createTemplate(inputTemplate)
        }
    }



    def setBindings(value) {
        bindingsMap = value
        expandTemplate()
    }

    def getBindings() {
        return bindingsMap
    }

    def expandTemplate() {
        def stringWriter = new StringWriter()
        template.make(bindings).writeTo(stringWriter)
        expandedTemplate = stringWriter.toString()
        expandedTemplateReader = new StringReader(expandedTemplate)
    }

    @Override
    void close() {
        expandedTemplateReader.close()
    }

    @Override
    void mark(int readAheadLimit) {
        expandedTemplateReader.mark(readAheadLimit)
    }

    @Override
    boolean markSupported() {
        return expandedTemplateReader.markSupported()
    }

    @Override
    int read() throws IOException {
        return expandedTemplateReader.read()
    }

    @Override
    int read(char[] cbuf, int off, int len) throws IOException {
        return expandedTemplateReader.read(cbuf, off, len)
    }

    @Override
    boolean ready() {
        return expandedTemplateReader.ready()
    }

    @Override
    void reset() {
        expandedTemplateReader.reset()
    }

    @Override
    long skip(long n) {
        return expandedTemplateReader.skip(n)
    }
}