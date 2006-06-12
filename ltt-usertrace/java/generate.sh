#!/bin/sh

#Sample
javac Sample.java
CLASSPATH=.:/usr/lib/jvm/java-1.5.0-sun-1.5.0.06/bin javah -jni Sample
gcc -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include \
  -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include/linux \
  -shared -Wl,-soname,libltt-java-string \
  -o libltt-java-string.so ltt-java-string.c \
  ../ltt-facility-loader-user_generic.c
LD_LIBRARY_PATH=. java Sample

#TestBrand
javac TestBrand.java
CLASSPATH=.:/usr/lib/jvm/java-1.5.0-sun-1.5.0.06/bin javah -jni TestBrand
gcc -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include \
  -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include/linux \
  -shared -Wl,-soname,libltt-java-thread_brand \
  -o libltt-java-thread_brand.so ltt-java-thread_brand.c \
  ../ltt-facility-loader-user_generic.c
LD_LIBRARY_PATH=. java TestBrand
