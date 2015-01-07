# NuoDB PHP PDO Native Driver #

[![Build Status](https://api.travis-ci.org/nuodb/nuodb-php-pdo.png?branch=master)](http://travis-ci.org/nuodb/nuodb-php-pdo)

## Installing Necessary Dependencies ##

In order to build the driver you must have PHP plus developer tools installed.
This may be done from source or using package managers.

Windows: PHP version 5.3 and 5.4 is required - with Thread Safety (TS)    
Linux:	PHP version 5.3, 5.4, or 5.5 is required - with Thread Safety (TS)  

NuoDB installed


## Build & Install on Unix ##

```bash
phpize --clean
phpize
configure --with-pdo-nuodb=/opt/nuodb
make clean
make
sudo make install
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

## Linux Driver Configuration ##

After the NuoDB PHP PDO Driver is installed, you must configure PHP to automatically load it.  The NuoDB PHP PDO Driver is a PHP extension library.  The NuoDB PHP PDO Driver extension library depends on the PHP PDO extension library, and must be loaded after it.  Otherwise you will get an error message when PHP attempts to load.

If your PHP installation has a configuration that exists entirely in a single PHP initization file (php.ini), then add the following to that php.ini, after the PHP PDO extension library is loaded:

  extension=pdo_nuodb.so

Your PHP installation may have a configuration that does not exist entirely in a single PHP initalization file (php.ini).  Some PHP installations are set to scan an additional directory for initialization files.  For example, on Ubuntu 14.04, the command line version of PHP is coded to scan the /etc/php5/cli/conf.d directory for initalization files.  In those cases, you should create your own initialziation file (e.g. pdo_nuodb.ini) file which loads the NuoDB PHP PDO Extension library:

  extension=pdo_nuodb.so

and make sure that pdo_nuodb.ini loaded after the PHP PDO extension library (pdo.so).


## RUNNING TESTS ##

Prerequisites for running the unit tests include having a running database at test@localhost.  The privileged credentials must be the username/password: "dba/dba".  The non-privileged credentials must be username/password: "cloud/user". The database can be started using these commands on Unix:

```bash
java -jar /opt/nuodb/jar/nuoagent.jar --broker --domain test --password bird --bin-dir /opt/nuodb/bin &
nuodbmgr --broker localhost --password bird --command "start process sm host localhost database test archive /tmp/nuodb_test_data waitForRunning true initialize true"
nuodbmgr --broker localhost --password bird --command "start process te host localhost database test options '--dba-user dba --dba-password dba'"
echo "create user cloud password 'user';" | /opt/nuodb/bin/nuosql test@localhost --user dba --password dba
```

Run the following commands to run the tests:

```bash
pear run-tests tests/*.phpt
```

## LOGGING ##

You can optionally enable and control logging with the following PHP configuration variables:
Add the following to your loaded php.ini 

  **pdo_nuodb.enable_log**   
  **pdo_nuodb.log_level**     
  **pdo_nuodb.logfile_path**  

**pdo_nuodb.enable_log** defaults to 0.  To enable logging, set **pdo_nuodb.enable_log=1**.

**pdo_nuodb.log_level** defaults to 1.  You can use levels 1-5. The higher level numbers have more detail.  The higher level numbers include lesser levels:

  1 - errors/exceptions only  
  2 - SQL statements  
  3 - API   
  4 - Functions   
  5 - Everything  

**pdo_nuodb.logfile_path** defaults to /tmp/nuodb_pdo.log.  You can override that default by specifying your own path.



[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/eebba2b3f495d19d760a0b42e0ce67fd "githalytics.com")](http://githalytics.com/nuodb/nuodb-php-pdo)
