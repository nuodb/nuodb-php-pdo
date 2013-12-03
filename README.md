# NuoDB PHP PDO Native Driver #

[![Build Status](https://api.travis-ci.org/nuodb/nuodb-php-pdo.png?branch=master)](http://travis-ci.org/nuodb/nuodb-php-pdo)

## Installing Necessary Dependencies ##

In order to build the driver you must have PHP plus developer tools installed.
This may be done from source or using package managers.

n.b. PHP version 5.3 or 5.4 is required. 
n.b. NuoDB installed

## Build & Install on Unix ##

```bash
phpize --clean
phpize
configure --with-pdo-nuodb=/opt/nuodb
make clean
make
make install
```


## Build on Windows ##


```cmd
phpize --clean
phpize
configure --with-pdo-nuodb="C:\Program Files\NuoDB"
nmake clean
nmake
```

## Windows Install Example with PHP installed in C:\php ##

```cmd
php -i | findstr extension_dir
del /S /Q C:\php\php_pdo_nuodb.*
copy Release_TS\php_pdo_nuodb.* C:\php
```



## RUNNING TESTS ##

Prerequisites for running the unit tests include having a running chorus; a
minimal chorus can be started using these commands:

```bash
java -jar nuoagent.jar --broker --domain test --password test &
./nuodb --chorus flights --password planes --dba-user dba --dba-password dba &
```

Run the following commands to run the tests:

```bash
pear run-tests tests/*.phpt
```


[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/eebba2b3f495d19d760a0b42e0ce67fd "githalytics.com")](http://githalytics.com/nuodb/nuodb-php-pdo)
