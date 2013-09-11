
How to build .jib files for use in an iButton for Java
======================================================

1. Make sure JiB.jar is in your CLASSPATH.

2. Use the correct .jibdb file.
Use javaone.jibdb for the rings handed out at JavaOne.

3. Invoke BuildJiBlet with the input file or files, the output
file, and the JiB database file you wish to use.

ex.

java BuildJiBlet -f file1.class -f file2.class -o outfile.jib -d javaone.jibdb


