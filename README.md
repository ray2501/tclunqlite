tclunqlite
=====

This is the UnQLite extension for Tcl using the Tcl Extension
Architecture (TEA).  For additional information on UnQLite see

[https://unqlite.symisc.net/](https://unqlite.symisc.net/)


UNIX BUILD
=====

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the tcl/unix/README file in the Tcl src dist. The following minimal
example will install the extension in the /opt/tcl directory.

	$ cd tclunqlite
	$ ./configure --prefix=/opt/tcl
	$ make
	$ make install
	
If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

	$ cd tclunqlite
	$ ./configure --with-tcl=/opt/activetcl/lib
	$ make
	$ make install

UnQLite is threadsafe and re-entrant (but it is not the default behavior).
To compile UnQLite with threading support, define the UNQLITE_ENABLE_THREADS
compile-time directive.

So at v0.2.5, I add UNQLITE_ENABLE_THREADS flag in configure.ac.
If you don't need enable this, modified configure.ac to remove this flag,
from TEA_ADD_CFLAGS([-DUNQLITE_ENABLE_THREADS]) to TEA_ADD_CFLAGS([]),
then do autoconf to create a new configure file.
below is an example:

	$ cd tclunqlite
	$ autoconf
	$ ./configure --prefix=/opt/tcl
	$ make
	$ make install

    
WINDOWS BUILD
=====

The recommended method to build extensions under windows is to use the
Msys + Mingw build process. This provides a Unix-style build while
generating native Windows binaries. Using the Msys + Mingw build tools
means that you can use the same configure script as per the Unix build
to create a Makefile.


Implement commands
=====

The interface to the UnQLite library consists of single tcl command named 
unqlite. Once an UnQLite database is open, it can be controlled using 
methods of the DBNAME command.

The key is interpreted by Tcl as a string and data is interpreted by Tcl as 
a string or byte array (-binary BOOLEAN flag).

### Basic usage

unqlite DBNAME FILENAME ?-readonly BOOLEAN? ?-mmap BOOLEAN? ?-create BOOLEAN? ?-in-memory BOOLEAN? ?-nomutex BOOLEAN?  
unqlite -enable-threads  
DBNAME close  
DBNAME config ?-disableautocommit BOOLEAN?  

### Key/value features

DBNAME kv_store key value ?-binary BOOLEAN?  
DBNAME kv_append key value ?-binary BOOLEAN?  
DBNAME kv_fetch key ?-binary BOOLEAN?  
DBNAME kv_delete key  

### Transactions

DBNAME begin  
DBNAME commit  
DBNAME rollback  

### Cursors

DBNAME cursor_init CURSORNAME  
CURSORNAME seek key pos  
CURSORNAME first  
CURSORNAME last  
CURSORNAME next  
CURSORNAME prev  
CURSORNAME isvalid  
CURSORNAME getkey  
CURSORNAME getdata ?-binary BOOLEAN?  
CURSORNAME delete  
CURSORNAME reset  
CURSORNAME release  

### Document Store (JSON via Jx9) Interfaces

DBNAME doc_create collection_name  
DBNAME doc_fetch  
DBNAME doc_fetch_id record_id  
DBNAME doc_fetchall  
DBNAME doc_store json_record  
DBNAME doc_update_record record_id json_record  
DBNAME doc_count  
DBNAME doc_delete record_id  
DBNAME doc_reset_cursor  
DBNAME doc_current_id  
DBNAME doc_last_id  
DBNAME doc_begin  
DBNAME doc_commit  
DBNAME doc_rollback  
DBNAME doc_drop  
DBNAME doc_close  

### Document Store (JSON via Jx9) Interfaces, for JX9 script

DBNAME jx9_eval Jx9_script_string  
DBNAME jx9_eval_file Jx9_script_file  

### Misc

DBNAME random_string buf_size  
DBNAME version   


Examples
=====

### Check UnQLite thread support (add in v0.2.5)

    package require unqlite

    #true -> enable, false -> not enable
    unqlite -enable-threads

### Get UnQLite version

    package require unqlite

    unqlite db1 ":mem:" -in-memory 1
    puts [db1 version]
    db1 close

### Key/Value store

    package require unqlite

    unqlite db1 "test.db"
    for {set i 1} {$i <= 100} {incr i} {
        set key [db1 random_string 4]
        set value [db1 random_string 20]
        
        db1 kv_store $key $value
    }

    db1 cursor_init cursor1
    for {cursor1 first} {[cursor1 isvalid]} {cursor1 next} {
        set key [cursor1 getkey]
        set data [cursor1 getdata]
        
        set outputkey "Get key: "
        append outputkey $key
        puts $outputkey
        
        set outputdata "Get data: "
        append outputdata $data
        puts $outputdata
    }

    cursor1 release
    db1 close

### Key/Value store - transactions

    package require unqlite
    unqlite db1 "test.db"

    db1 kv_store "foo" "bar"
    puts [db1 kv_fetch "foo"]

    # commit to database
    db1 commit

    db1 begin
    db1 kv_store "test" "testmessage"
    db1 kv_delete "foo"
    puts [db1 kv_fetch "foo"]
    puts [db1 kv_fetch "test"]
    db1 rollback
    puts [db1 kv_fetch "foo"]
    puts [db1 kv_fetch "test"]

    db1 close

### Key/Value store (binary value mode)
 
    package require unqlite  
    
    unqlite Db1 "test.db"  
    set filename "unqlite-db-116.zip"  

    set size [file size "/home/danilo/Downloads/unqlite-db-116.zip"] 
    set fd [open "/home/danilo/Downloads/unqlite-db-116.zip" {RDWR BINARY}]  
    fconfigure $fd -blocking 1 -encoding iso8859-1 -translation binary 
    set data [read $fd $size]  
    close $fd  
    Db1 kv_store $filename $data -binary 1

    set fetch_data [Db1 kv_fetch $filename -binary 1]
    set fd [open "/home/danilo/Downloads/unqlite-db-116_test.zip" {CREAT RDWR BINARY}]  
    fconfigure $fd -blocking 1 -encoding iso8859-1 -translation binary 
    puts -nonewline $fd $fetch_data  
    close $fd  

    Db1 close

### Key/Value store (binary value mode) - 2

    package require unqlite  
    
    unqlite Db1 "test.db"  
    set filename "unqlite-db-116.zip"  

    set size [file size "/home/danilo/Downloads/unqlite-db-116.zip"] 
    set fd [open "/home/danilo/Downloads/unqlite-db-116.zip" {RDWR BINARY}]  
    fconfigure $fd -blocking 1 -encoding iso8859-1 -translation binary 
    set data [read $fd $size]  
    close $fd  
    Db1 kv_append $filename $data -binary 1

    Db1 cursor_init cursor1
    cursor1 seek $filename 0
    set fetch_data [cursor1 getdata -binary 1]
    set fd [open "/home/danilo/Downloads/unqlite-db-116_test.zip" {CREAT RDWR BINARY}]  
    fconfigure $fd -blocking 1 -encoding iso8859-1 -translation binary 
    puts -nonewline $fd $fetch_data  
    close $fd  

    cursor1 release
    Db1 close

### Document store

    package require unqlite

    unqlite db1 "test.db"

    db1 doc_create user
    db1 doc_store "{ name : 'alex', age : 19, mail : 'alex@example.com'  }"
    db1 doc_store "{ name : 'robert', age : 49, mail : 'dude@example.com'  }"
    db1 doc_store "{ name : 'orange', age : 12, mail : 'cat@example.com'  }"
    db1 doc_store "{ name : 'michael', age : 52, mail : 'test@example.com'  }"
    db1 doc_store "{ name : 'glary', age : 45, mail : 'beart@example.com'  }"

    puts [db1 doc_fetchall]
    db1 doc_update_record 1 "{ name : 'scott', age : 49, mail : 'dude@test.com'  }"
    puts "Now doc_fetc_id and doc_delete id 1"
    puts [db1 doc_fetch_id 1]
    db1 doc_delete 1
    puts "After doc_delete..."
    puts [db1 doc_fetchall]
    db1 doc_close

    db1 close

### Eval a JX9 script string

    package require unqlite

    unqlite db1 "test.db"

    db1 jx9_eval { $rc = db_exists("user");if (!$rc) {$rc = db_create("user");} print $rc }
    db1 jx9_eval {print db_store('user', { name : 'alex', age : 19, mail : 'alex@example.com'  });}
    db1 jx9_eval {print db_store('user', { name : 'robert', age : 49, mail : 'dude@example.com'  });}
    db1 jx9_eval {print db_store('user', { name : 'orange', age : 12, mail : 'cat@example.com'  });}
    db1 jx9_eval {print db_store('user', { name : 'michael', age : 52, mail : 'test@example.com'  });}
    db1 jx9_eval {print db_store('user', { name : 'glary', age : 45, mail : 'beart@example.com'  });}

    puts [db1 jx9_eval {print db_fetch_all('user');}]

    db1 close

### Eval a JX9 script file

config_writer.jx9 -

    // Create a dummy JSON object configuration
    $my_config =  {
    bind_ip : "127.0.0.1",
    config : "/etc/symisc/jx9.conf",
    dbpath : "/usr/local/unqlite",
    fork : true,
    logappend : true,
    logpath : "/var/log/jx9.log",
    quiet : true,
    port : 8080
    };

    // Dump the JSON object
    print $my_config;

    // Write to disk
    $file = "config.json.txt";
    print "\n\n------------------\nWriting JSON object to disk file: '$file'\n";

    // Create the file
    $fp = fopen($file,'w+');
    if( !$fp )
        exit("Cannot create $file");

    // Write the JSON object
    fwrite($fp,$my_config);

    // Close the file
    fclose($fp);

    // All done
    print "$file successfully created on: "..__DATE__..' '..__TIME__;

    // ---------------------- config_reader.jx9 ----------------------------

    // Read the configuration file defined above
    $iSz = filesize($zFile); // Size of the whole file
    $fp = fopen($zFile,'r'); // Read-only mode
    if( !$fp ){
    exit("Cannot open '$zFile'");
    }

    // Read the raw content
    $zBuf = fread($fp,$iSz);

    // decode the JSON object now
    $my_config = json_decode($zBuf);
    if( !$my_config ){
        print "JSON decoding error\n";
    }else{
    // Dump the JSON object
    foreach( $my_config as $key,$value ){
    print "$key ===> $value\n";
    }
    }

    // Close the file
    fclose($fp);

Example -

    package require unqlite

    unqlite db1 ":mem:" -in-memory 1
    # for Windows
    #db1 jx9_eval_file "c:/config_writer.jx9"
    # for Linux
    db1 jx9_eval_file "/home/danilo/config_writer.jx9"

    db1 close


Notes
=====

DBNAME doc_current_id always return current record id 0, and DBNAME 
doc_fetch only can get the first record, this is UnQLite API limits.

