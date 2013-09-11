@echo off
rem
rem This batch file assumes Java version 1.2 or greater.
rem If you use 1.1 this batch file will work if you skip
rem (jump over) the javac step!!!
rem
echo .

if "%1" == "" goto nofile
if exist %1\NUL goto testver
goto badfile

:testver
if "%2" == "" goto noversion
if "%2" == "0003" goto ver0003
if "%2" == "0004" goto ver0004
if "%2" == "0005" goto ver0005
if "%2" == "0006" goto ver0006
if "%2" == "0007" goto ver0007
if "%2" == "0008" goto ver0008
echo Bad version specified
goto usage

:ver0003
SET JIBDB=javaone.jibdb
SET JIBAPI=iButton32.jar
Echo Using FVS "Java iButton Firmware Version 0.03.0003"
goto dobuild

:ver0004
SET JIBDB=jib33.jibdb
SET JIBAPI=iButton33.jar
Echo Using FVS "iButton with Java - Firmware Version 1.00.0004"
goto dobuild

:ver0005
SET JIBDB=jib35.jibdb
SET JIBAPI=iButton35.jar
Echo Using FVS "iButton with Java - Firmware Version 1.01.0005"
goto dobuild

:ver0006
SET JIBDB=jib51.jibdb
SET JIBAPI=iButton51.jar
Echo Using FVS "iButton with Java - Firmware Version 1.10.0006" OR
Echo           "iButton with Java - Firmware Version 1.11.0006" OR
Echo           "Java Powered iButton - Firmware Version 2.00.0006"
goto dobuild

:ver0007
SET JIBDB=jib52.jibdb
SET JIBAPI=iButton52.jar
Echo Using FVS  Java Powered iButton - Firmware Version 2.2.0007
goto dobuild

:ver0008
SET JIBDB=jib53.jibdb
SET JIBAPI=iButton53.jar
Echo Using FVS  Java Powered iButton - Firmware Version 2.21.0008
goto dobuild

:dobuild
if exist %1\%2\NUL goto havedir
mkdir %1\%2

:havedir
rem
rem uncomment this to skip applet compile step
rem
rem goto buildjib

cd %1

:mjrhack1
  echo .
  echo Performing major hack to get around JDKs insistance that 
  echo java.lang.Error must exist even when we use the bootclasspath
  echo switch!!!
  echo .
  echo If you use another Java compiler you can jump over this (javac) step.
  echo .
  mkdir java
  cd java
  mkdir lang
  cd lang
  echo package java.lang; >Error.java
  echo public class Error { >>Error.java
  echo }>>Error.java
  cd ..\..
:endhack1

del *.class
echo Running javac...
javac  -bootclasspath ..\%JIBAPI% *.java
echo ...javac Done

:mjrhack2
  deltree /y java >NUL
:endhack2

cd ..

:buildjib
del  %1\%2\%1.jib
java -classpath ..\..\DevTools\BuildJiBlet\JiB.jar;%CLASSPATH% BuildJiBlet -f %1 -o %1\%2\%1.jib -d %JIBDB%
goto end

:noversion
echo No firmware version specified
goto usage

:badfile
echo %1 not found
goto usage

:nofile
echo No applet directory specified

:usage                                  
echo USAGE gojib "exampleFolder Name" "last 4 digits of Firmware String"

:end
echo .

