package org.opengee.os.package

import java.util.regex.Matcher
import java.util.regex.Pattern


// A class that stores information about a dependency.
// (E.g., 'perl >= 1:5')
@groovy.transform.InheritConstructors
class Dependency extends org.redline_rpm.Dependency {
    static Dependency fromSpecifier(String dependencySpecifier) {
        Matcher m = Pattern.compile('[<=>]+').matcher(dependencySpecifier)
        String name
        String version
        int flags

        // Parse strings like 'name(arch) <=> 1.0+3' (returned from commands
        // like `find-requires`):
        if (m.find()) {
            name = dependencySpecifier.substring(0, m.start()).trim()
            flags = 0
            m.group().each {
                switch (it) {
                    case '<':
                        flags |= org.freecompany.redline.header.Flags.LESS
                        break
                    case '=':
                        flags |= org.freecompany.redline.header.Flags.EQUAL
                        break
                    case '>':
                        flags |= org.freecompany.redline.header.Flags.GREATER
                        break
                }
            }
            version = dependencySpecifier.substring(m.end()).trim()
        } else {
            name = dependencySpecifier
            version = null
            flags = 0
        }

        return new Dependency(name, version, flags)
    }

    String getComparisonString() {
        return """${
            flags & org.freecompany.redline.header.Flags.LESS    ? '<' : '' }${
            flags & org.freecompany.redline.header.Flags.GREATER ? '>' : '' }${
            flags & org.freecompany.redline.header.Flags.EQUAL   ? '=' : '' }"""
    }

    // Make Dependency hashable:
    public boolean equals(Object comparand) {
        if (!(comparand instanceof Dependency)) {
            return false
        } else {
            Dependency that = (Dependency)comparand

            return this.name.equals(that.name) &&
                this.version.equals(that.version) &&
                this.flags.equals(that.flags)
        }
    }

    // Make Dependency hashable:
    public int hashCode() {
        int result = this.name.hashCode()

        if (this.version != null) {
            result = result * 23 + this.version.hashCode()
        }
        result = result * 31 + this.flags.hashCode()

        return result;
    }

    public String toString() {
        // '<=>':
        String comparison = getComparisonString()
        // ' <=> 1.0+etc.version':
        String comparisonVersion =
            version != null && version != '' && comparison != '' ?
                " ${comparison} ${version}" :
                ""

        return "${name}${comparisonVersion}"
    }
}
