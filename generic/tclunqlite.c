/*
 * Copyright (c) <2015>, <Danilo Chang>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unqlite.h"


/*
 * Windows needs to know which symbols to export.  Unix does not.
 * BUILD_unqlite should be undefined for Unix.
 */
#ifdef BUILD_unqlite
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_unqlite */


typedef struct UnqliteDb UnqliteDb;


struct UnqliteDb {
  unqlite *db;                /* A fresh UnQlite database connection handle. */
  Tcl_Interp *interp;
  unqlite_kv_cursor *cursor;  /* A database cursor handle. */
  char *cursor_command;
  unqlite_vm *vm;             /* A compiled Jx9 program represented */
  char *collection_name;      /* Collection name, for document store used */
};


/*
** TCL calls this procedure when a unqlite cursors command is
** deleted.
*/
static void CursorDeleteCmd(void *db) {
  UnqliteDb *pDb = (UnqliteDb*)db;

  unqlite_kv_cursor_release(pDb->db, pDb->cursor);
  pDb->cursor = 0;

  if(pDb->cursor_command) {
    Tcl_Free(pDb->cursor_command);
    pDb->cursor_command = 0;
  }
}


/*
** TCL calls this procedure when a unqlite database command is
** deleted.
*/
static void DbDeleteCmd(void *db) {
  UnqliteDb *pDb = (UnqliteDb*)db;
  Tcl_Obj *cursor_cmd;

  if(pDb->collection_name) {
    Tcl_Free(pDb->collection_name);
    pDb->collection_name = 0;
  }

  if(pDb->cursor_command) {
    /* When delete database command, also remove cursor command */
    cursor_cmd = Tcl_NewStringObj(pDb->cursor_command, -1);
    Tcl_DeleteCommand(pDb->interp, Tcl_GetStringFromObj(cursor_cmd, 0));

    Tcl_Free(pDb->cursor_command);
    pDb->cursor_command = 0;
  }
  //if(pDb->cursor) {
  //  unqlite_kv_cursor_release(pDb->db, pDb->cursor);
  //  pDb->cursor = 0;
  //}

  pDb->vm = 0;

  unqlite_close(pDb->db);
  pDb->db = 0;

  Tcl_Free((char*)pDb);
  pDb = 0;
}


/*
** Creates a new Tcl command for unqlite cursors.
*/
static int CursorObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  UnqliteDb *pDb = (UnqliteDb *) cd;
  int choice;
  int result;
  int rc = TCL_OK;

  static const char *CURSOR_strs[] = {
    "seek",
    "first",
    "last",
    "next",
    "prev",
    "isvalid",
    "getkey",
    "getdata",
    "delete",
    "reset",
    "release",
    0
  };

  enum CURSOR_enum {
    CURSOR_SEEK,
    CURSOR_FIRST,
    CURSOR_LAST,
    CURSOR_NEXT,
    CURSOR_PREV,
    CURSOR_ISVALID,
    CURSOR_GETKEY,
    CURSOR_GETDATA,
    CURSOR_DELETE,
    CURSOR_RESET,
    CURSOR_RELEASE,
  };

  if( objc < 2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
    return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], CURSOR_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  switch( (enum CURSOR_enum) choice ){
    case CURSOR_SEEK: {
      char *zKey;
      int len;
      int iPos;

      if( objc == 4 ){
        zKey = Tcl_GetStringFromObj(objv[2], &len);
        if( !zKey || len < 1 ){
          return TCL_ERROR;
        }

        Tcl_GetIntFromObj(interp, objv[3], &iPos);
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "key pos");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_seek(pDb->cursor, zKey, -1, iPos);
      if( result != UNQLITE_OK ){
	  Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	  return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_FIRST: {
      if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "first");
	    return TCL_ERROR;
      }

      result = unqlite_kv_cursor_first_entry(pDb->cursor);
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_LAST: {
      if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "last");
	    return TCL_ERROR;
      }

      result = unqlite_kv_cursor_last_entry(pDb->cursor);
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_NEXT: {
      if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "next");
	    return TCL_ERROR;
      }

      result = unqlite_kv_cursor_next_entry(pDb->cursor);
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_PREV: {
      if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "prev");
	    return TCL_ERROR;
      }

      result = unqlite_kv_cursor_prev_entry(pDb->cursor);
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_ISVALID: {
      if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "isvalid");
	    return TCL_ERROR;
      }

      result = unqlite_kv_cursor_valid_entry(pDb->cursor);

      // return 1 when valid. 0 otherwise
      if( result != 1 ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_GETKEY: {
      char *zKey;
      int nBytes;
      Tcl_Obj *pResultStr;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "getkey");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_key(pDb->cursor, NULL, &nBytes);
      if( result != UNQLITE_OK ){
        Tcl_SetResult (interp, "Get key size first fail", NULL);
        return TCL_ERROR;
      }

      //We need add a null char space to get correct string
      nBytes = nBytes + sizeof(char);
      zKey = (char *)malloc(nBytes);
      if( zKey == NULL ) {
        Tcl_SetResult (interp, "Get key but empty string - fail", NULL);

        return TCL_ERROR;
      }

      unqlite_kv_cursor_key(pDb->cursor, zKey, &nBytes);
      if( result != UNQLITE_OK ){
        free(zKey);

        Tcl_SetResult (interp, "Get key fail", NULL);
        return TCL_ERROR;
      }

      pResultStr = Tcl_NewStringObj(zKey, -1);
      free(zKey);

      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

    case CURSOR_GETDATA: {
      char *zData = NULL;
      unsigned char *zData_binary = NULL;
      signed long long int nBytes;
      Tcl_Obj *pResultStr;
      char *zArg;
      int binary_mode = 0;

      if( objc == 2 || objc == 4){
        if( objc == 4 ) {
          zArg = Tcl_GetStringFromObj(objv[2], 0);

          if( strcmp(zArg, "-binary")==0 ){
            if( Tcl_GetBooleanFromObj(interp, objv[3], &binary_mode) ) return TCL_ERROR;
          }else{
            Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
            return TCL_ERROR;
          }
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "getdata ?-binary BOOLEAN? ");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_data(pDb->cursor, NULL, &nBytes);
      if( result != UNQLITE_OK ){
        Tcl_SetResult (interp, "Get data size first fail", NULL);
        return TCL_ERROR;
      }

      //We need add a null char space to get correct string
      if(binary_mode==0) {
        nBytes = nBytes + sizeof(char);
        zData = (char *)malloc(nBytes);
        if( zData == NULL ) {
          Tcl_SetResult (interp, "Get data but empty string - fail", NULL);

          return TCL_ERROR;
        }
      } else {
        zData_binary = (unsigned char *)malloc(nBytes);
        if( zData_binary == NULL ) {
          Tcl_SetResult (interp, "Get binary data but empty - fail", NULL);

          return TCL_ERROR;
        }
      }

      if(binary_mode==0) {
        unqlite_kv_cursor_data(pDb->cursor, zData, &nBytes);
      } else {
        unqlite_kv_cursor_data(pDb->cursor, zData_binary, &nBytes);
      }
      if( result != UNQLITE_OK ){
        if(zData) free(zData);
        if(zData_binary) free(zData_binary);

        Tcl_SetResult (interp, "Get data fail", NULL);
        return TCL_ERROR;
      }

      if(binary_mode==0) {
        pResultStr = Tcl_NewStringObj(zData, -1);
        free(zData);
      } else {
        pResultStr = Tcl_NewByteArrayObj(zData_binary, nBytes);
        free(zData_binary);
      }

      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

    case CURSOR_DELETE: {
      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "delete");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_delete_entry(pDb->cursor);
      if( result != UNQLITE_OK ){
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case CURSOR_RESET: {
      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "reset");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_reset(pDb->cursor);
      if( result != UNQLITE_OK ){
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    /*    $cursor release
    **
    ** Release the database cursor
    */
    case CURSOR_RELEASE: {
      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      if(pDb->cursor_command) {
        Tcl_Free(pDb->cursor_command);
        pDb->cursor_command = 0;
      }

      break;
    }
  } /* End of the SWITCH statement */

  return rc;
}


/*
** The "unqlite" command below creates a new Tcl command for each
** connection it opens to an UnQLite database.  This routine is invoked
** whenever one of those connection-specific commands is executed
** in Tcl.  For example, if you run Tcl code like this:
**
**       unqlite db1  "my_database"
**       db1 close
**
** The first command opens a connection to the "my_database" database
** and calls that connection "db1".  The second command causes this
** subroutine to be invoked.
*/
static int DbObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  UnqliteDb *pDb = (UnqliteDb *) cd;
  int choice;
  int result;
  int rc = TCL_OK;

  static const char *DB_strs[] = {
    "kv_store",          // Saves entry
    "kv_append",         // Append entry data specified by key
    "kv_fetch",          // Fetch data specified by key
    "kv_delete",         // Delete entry specified by key
    "begin",             // Manual Transaction Manager
    "commit",            // Manual Transaction Manager
    "rollback",          // Manual Transaction Manager
    "config",
    "close",             // Close database
    "cursor_init",       // Create a cursor command
    "random_string",     // Generate a random string
    "version",
    "doc_create",        // Store (JSON via Jx9) Interfaces
    "doc_fetch",
    "doc_fetchall",
    "doc_fetch_id",
    "doc_store",
    "doc_delete",
    "doc_reset_cursor",
    "doc_count",
	"doc_current_id",
	"doc_last_id",
    "doc_begin",
    "doc_commit",
    "doc_rollback",
    "doc_drop",
    "doc_close",
    "jx9_eval",          // Execute a JX9 script string
    "jx9_eval_file",     // Execute a JX9 script from file
    0
  };

  enum DB_enum {
    DB_KV_STORE,
    DB_KV_APPEND,
    DB_KV_FETCH,
    DB_KV_DELETE,
    DB_BEGIN,
    DB_COMMIT,
    DB_ROLLBACK,
    DB_CONFIG,
    DB_CLOSE,
    DB_CURSOR_INIT,
    DB_RANDOM_STRING,
    DB_VERSION,
    DB_DOC_CREATE,
    DB_DOC_FETCH,
    DB_DOC_FETCHALL,
    DB_DOC_FETCH_ID,
    DB_DOC_STORE,
    DB_DOC_DELETE,
    DB_DOC_RESET_CURSOR,
    DB_DOC_COUNT,	
    DB_DOC_CURRENT_ID,
    DB_DOC_LAST_ID,
    DB_DOC_BEGIN,
    DB_DOC_COMMIT,
    DB_DOC_ROLLBACK,
    DB_DOC_DROP,
    DB_DOC_CLOSE,
    DB_JX9_EVAL,
    DB_JX9_EVAL_FILE,
  };

  if( objc < 2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
    return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], DB_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  switch( (enum DB_enum)choice ){

    case DB_KV_STORE: {
      char *zKey;
      char *zData = NULL;
      unsigned char *zData_binary = NULL;
      int len;
      char *zArg;
      int binary_mode = 0;

      if( objc == 4 || objc == 6){
        zKey = Tcl_GetStringFromObj(objv[2], &len);
        if( !zKey || len < 1 ){
          return TCL_ERROR;
        }

        if( objc == 6 ) {
          zArg = Tcl_GetStringFromObj(objv[4], 0);

          if( strcmp(zArg, "-binary")==0 ){
            if( Tcl_GetBooleanFromObj(interp, objv[5], &binary_mode) ) return TCL_ERROR;
          }else{
            Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
            return TCL_ERROR;
          }
        }

        if(binary_mode) {
	      zData_binary = Tcl_GetByteArrayFromObj(objv[3], &len);
  	      if( !zData_binary || len < 1 ){
	        return TCL_ERROR;
	      }
        } else {
	      zData = Tcl_GetStringFromObj(objv[3], &len);
  	      if( !zData || len < 1 ){
	        return TCL_ERROR;
	      }
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "key value ?-binary BOOLEAN? ");
        return TCL_ERROR;
      }

      /* If the nKeyLen argument is less than zero,
         then pKey is read up to the first zero terminator.*/
      if(binary_mode) {
        result = unqlite_kv_store(pDb->db, zKey, -1, zData_binary, len);
      } else {
        result = unqlite_kv_store(pDb->db, zKey, -1, zData, strlen(zData) + sizeof(char));
      }

      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_ERROR;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));
      break;
    }

    case DB_KV_APPEND: {
      char *zKey;
      char *zData = NULL;
      unsigned char *zData_binary = NULL;
      int len;
      char *zArg;
      int binary_mode = 0;

      if( objc == 4 || objc == 6){
        zKey = Tcl_GetStringFromObj(objv[2], &len);
        if( !zKey || len < 1 ){
          return TCL_ERROR;
        }

        if( objc == 6 ) {
          zArg = Tcl_GetStringFromObj(objv[4], 0);

          if( strcmp(zArg, "-binary")==0 ){
            if( Tcl_GetBooleanFromObj(interp, objv[5], &binary_mode) ) return TCL_ERROR;
          }else{
            Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
            return TCL_ERROR;
          }
        }

        if(binary_mode) {
          zData_binary = Tcl_GetByteArrayFromObj(objv[3], &len);
          if( !zData_binary || len < 1 ){
            return TCL_ERROR;
          }
        } else {
          zData = Tcl_GetStringFromObj(objv[3], &len);
          if( !zData || len < 1 ){
            return TCL_ERROR;
          }
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "key value ?-binary BOOLEAN?");
        return TCL_ERROR;
      }

      /* If the nKeyLen argument is less than zero,
         then pKey is read up to the first zero terminator.*/
      if(binary_mode) {
        result = unqlite_kv_append(pDb->db, zKey, -1, zData_binary, len);
      } else {
        result = unqlite_kv_append(pDb->db, zKey, -1, zData, strlen(zData) + sizeof(char));
      }
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_ERROR;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));
      break;
    }

    case DB_KV_FETCH: {
      char *zKey;
      int len;
      char *zBuf = NULL;
      unsigned char *zBuf_binary = NULL;
      signed long long int nBytes;
      char *zArg;
      int binary_mode = 0;
      Tcl_Obj *pResultStr;

      if( objc == 3 || objc == 5 ){
        zKey = Tcl_GetStringFromObj(objv[2], &len);
        if( !zKey || len < 1 ){
          return TCL_ERROR;
        }

        if( objc == 5 ) {
          zArg = Tcl_GetStringFromObj(objv[3], 0);

          if( strcmp(zArg, "-binary")==0 ){
            if( Tcl_GetBooleanFromObj(interp, objv[4], &binary_mode) ) return TCL_ERROR;
          }else{
            Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
            return TCL_ERROR;
          }
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "key ?-binary BOOLEAN? ");
        return TCL_ERROR;
      }

      result = unqlite_kv_fetch(pDb->db, zKey, -1, NULL, &nBytes);
      if( result != UNQLITE_OK ){
        //Tcl_SetResult (interp, "Extract data size first fail", NULL);
        Tcl_SetResult (interp, "", NULL);
	    return TCL_OK;
      }

      //We need add a null char space to get correct string if binary_mode != 1
      if(binary_mode == 0) {
        nBytes = nBytes + sizeof(char);
        zBuf = (char *)malloc(nBytes);
        if( zBuf == NULL ) {
          Tcl_SetResult (interp, "Malloc memory fail", NULL);
          return TCL_ERROR;
        }
      } else {
        zBuf_binary = (unsigned char *)malloc(nBytes);
        if( zBuf_binary == NULL ) {
          Tcl_SetResult (interp, "Malloc memory fail", NULL);
          return TCL_ERROR;
        }
      }

      if(binary_mode == 0) {
        result = unqlite_kv_fetch(pDb->db, zKey, -1, zBuf, &nBytes);
      } else {
        result = unqlite_kv_fetch(pDb->db, zKey, -1, zBuf_binary, &nBytes);
      }
      if( result != UNQLITE_OK ){
        if(zBuf) free(zBuf);
        if(zBuf_binary) free(zBuf_binary);

        Tcl_SetResult (interp, "Fetch data fail", NULL);
        return TCL_ERROR;
      }

      if(binary_mode == 0) {
        pResultStr = Tcl_NewStringObj(zBuf, -1);
        free(zBuf);
      } else {
        pResultStr = Tcl_NewByteArrayObj(zBuf_binary, nBytes);
        free(zBuf_binary);
      }

      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

    case DB_KV_DELETE: {
      char *zKey;
      int len;

      if( objc == 3 ){
        zKey = Tcl_GetStringFromObj(objv[2], &len);
        if( !zKey || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "key");
        return TCL_ERROR;
      }

      result = unqlite_kv_delete(pDb->db, zKey, -1);
      if( result != UNQLITE_OK ){
	  Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	  return TCL_ERROR;
      }

     Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case DB_BEGIN: {
      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "begin");
        return TCL_ERROR;
      }

      result = unqlite_begin(pDb->db);
      if( result != UNQLITE_OK ){
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case DB_COMMIT: {
      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "commit");
        return TCL_ERROR;
      }

      result = unqlite_commit(pDb->db);
      if( result != UNQLITE_OK ){
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case DB_ROLLBACK: {
     if( objc != 2 ){
	    Tcl_WrongNumArgs(interp, 1, objv, "rollback");
	    return TCL_ERROR;
      }

      result = unqlite_rollback(pDb->db);
      if( result != UNQLITE_OK ){
	    Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	    return TCL_OK;
      }

      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case DB_CONFIG: {
      char *zArg;
      int i;

      if( objc < 4 ){
        Tcl_WrongNumArgs(interp, 1, objv,
                         "HANDLE config ?-disableautocommit BOOLEAN? ");
        return TCL_ERROR;
      }

      for(i=2; i+1<objc; i+=2){
        zArg = Tcl_GetStringFromObj(objv[i], 0);

        if( strcmp(zArg, "-disableautocommit")==0 ){
          int b;
          if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
          if( b ){
            unqlite_config(pDb->db, UNQLITE_CONFIG_DISABLE_AUTO_COMMIT);
          }
        }else{
          Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
          return TCL_ERROR;
        }
      }

      break;
    }

    /*    $db close
    **
    ** Shutdown the database
    */
    case DB_CLOSE: {
      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      break;
    }

    case DB_CURSOR_INIT: {
      char *zArg;
      int len;

      if( objc == 3 ){
        zArg = Tcl_GetStringFromObj(objv[2], &len);
        if( !zArg || len < 1 ){
           return TCL_ERROR;
        }

        pDb->cursor_command = Tcl_Alloc( len + 1 );
        memcpy(pDb->cursor_command, zArg, len+1);
      } else {
        Tcl_WrongNumArgs(interp, 2, objv, "cursor_name");
        return TCL_ERROR;
      }

      result = unqlite_kv_cursor_init(pDb->db, &pDb->cursor);
      if( result != UNQLITE_OK ){
	  Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
	  return TCL_ERROR;
      }

      Tcl_CreateObjCommand(interp, zArg, CursorObjCmd, (char *)pDb, CursorDeleteCmd);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(1));

      break;
    }

    case DB_RANDOM_STRING: {
      char *zBuf;
      int buf_size;
      Tcl_Obj *pResultStr;

      if( objc == 3 ){
        Tcl_GetIntFromObj(interp, objv[2], &buf_size);
      } else {
        Tcl_WrongNumArgs(interp, 2, objv, "buf_size");
        return TCL_ERROR;
      }

      zBuf = (char *)malloc(buf_size + sizeof(char));
      if( zBuf == NULL ) {
        Tcl_SetResult (interp, "Create random string but malloc memory fail", NULL);

        return TCL_ERROR;
      }

      /* Note that the generated string is not null terminated and the given buffer
       * must be big enough to hold at least 3 bytes.
       * So we malloc a char space and add a null char to our string
       */
      zBuf[buf_size] = '\0';
      unqlite_util_random_string(pDb->db, zBuf, buf_size);

      pResultStr = Tcl_NewStringObj(zBuf, -1);
      free(zBuf);

      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

    case DB_VERSION: {
      Tcl_SetResult(interp, (char *)unqlite_lib_version(), TCL_STATIC);

      break;
    }

    /*
     * If the given collection exists in the underlying database, this command still returns true.
     */
    case DB_DOC_CREATE: {
      const char *JX9_CREATE = "$rc = db_exists($argv[0]);if (!$rc) {$rc = db_create($argv[0]);}";
      char *collection_name;
      unqlite_value *value;
      int bool_result;
      int len;

      if( objc == 3 ){
        collection_name = Tcl_GetStringFromObj(objv[2], &len);

        if( collection_name && len > 0 ){
          pDb->collection_name = Tcl_Alloc( len + 1 );
          memcpy(pDb->collection_name, collection_name, len+1);
        }else{
          pDb->collection_name = 0;
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "collection_name");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_CREATE, strlen(JX9_CREATE),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    /*
     * Issue: UnQLite db_fetch function does not automatically advance the
     * cursor to the next record in the collection.
     *
     * If you use this command, looks like only get the current id data,
     * and you cannot get next record.
     */
    case DB_DOC_FETCH:
    case DB_DOC_FETCHALL: {

      const char *JX9_PROG = NULL;
  
      if( choice == DB_DOC_FETCH) {
        JX9_PROG = "print db_fetch($argv[0]);";
      } else if( choice == DB_DOC_FETCHALL) {
        JX9_PROG = "print db_fetch_all($argv[0]);";
      } 

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_PROG, strlen(JX9_PROG),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      const void *pOut = NULL;
      unsigned int nLen;
      char *iBuffer;
      Tcl_Obj *pResultStr = NULL;

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_EXTRACT_OUTPUT, &pOut, &nLen);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      if( nLen > 0 && pOut) {
        iBuffer = (char *) malloc (nLen + sizeof(char));
        sprintf(iBuffer, "%.*s", (int)nLen, (const char *)pOut);
        pResultStr = Tcl_NewStringObj(iBuffer, -1);
        free (iBuffer);
      } else if (nLen == 0) { /* Add nLen case to handle */
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }
    
    case DB_DOC_FETCH_ID: {
      const char *JX9_FETCH_ID = "print db_fetch_by_id($argv[0],$argv[1]);";
      char *record_id;
      int len;

      if( objc == 3 ){
        record_id = Tcl_GetStringFromObj(objv[2], &len);

        if( !record_id || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "record_id");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_FETCH_ID, strlen(JX9_FETCH_ID),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, record_id);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      const void *pOut = NULL;
      unsigned int nLen;
      char *iBuffer;
      Tcl_Obj *pResultStr = NULL;

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_EXTRACT_OUTPUT, &pOut, &nLen);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      if( nLen > 0 && pOut) {
        iBuffer = (char *) malloc (nLen + sizeof(char));
        sprintf(iBuffer, "%.*s", (int)nLen, (const char *)pOut);
        pResultStr = Tcl_NewStringObj(iBuffer, -1);
        free (iBuffer);
      } else if (nLen == 0) { /* Add nLen case to handle */
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }    

    case DB_DOC_STORE: {
      const char *JX9_STORE = "$rc = db_store($argv[0],$argv[1]);";
      char *json_record;
      int len;
      unqlite_value *value;
      int bool_result;

      if( objc == 3 ){
        json_record = Tcl_GetStringFromObj(objv[2], &len);

        if( !json_record || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "json_record");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_STORE, strlen(JX9_STORE),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, json_record);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    case DB_DOC_DELETE: {
      const char *JX9_DELETE = "$rc = db_drop_record($argv[0],$argv[1]);";
      char *record_id;
      int len;
      unqlite_value *value;
      int bool_result;

      if( objc == 3 ){
        record_id = Tcl_GetStringFromObj(objv[2], &len);

        if( !record_id || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "record_id");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_DELETE, strlen(JX9_DELETE),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, record_id);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    case DB_DOC_RESET_CURSOR: {
      /*
       * db_reset_record_cursor() reset the internal record cursor so that
       * a call to db_fetch() can re-start from the beginning.
       */
      const char *JX9_RESET_CURSOR = "$rc = db_reset_record_cursor($argv[0]);";
      unqlite_value *value;
      int bool_result;

      if( objc != 2 ){
          Tcl_WrongNumArgs(interp, 1, objv, "doc_reset_cursor");
          return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_RESET_CURSOR, strlen(JX9_RESET_CURSOR),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    case DB_DOC_COUNT:
    case DB_DOC_CURRENT_ID:
      /*
       * db_current_record_id() return the unique ID (64-bit integer) of
       * the record pointed by the internal cursor.
       */
    case DB_DOC_LAST_ID: {
      /*
       * db_last_record_id() return the unique ID (64-bit integer) of
       *  the last inserted record in the collection.
       */	  

      const char *JX9_PROG = NULL;
  
      if( choice == DB_DOC_CURRENT_ID) {
        JX9_PROG = "$rc = db_current_record_id($argv[0]);";
      } else if( choice == DB_DOC_LAST_ID) {
        JX9_PROG = "$rc = db_last_record_id($argv[0]);";
      } else if( choice == DB_DOC_COUNT) {
        JX9_PROG = "$rc = db_total_records($argv[0]);";		
	  }

      unqlite_value *value;
      signed long long int int64_result;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_PROG, strlen(JX9_PROG),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      int64_result = unqlite_value_to_int64(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewWideIntObj(int64_result));

      break;
    }

    case DB_DOC_BEGIN:
    case DB_DOC_COMMIT:
    case DB_DOC_ROLLBACK: {	  
      const char *JX9_PROG = NULL;
  
      if( choice == DB_DOC_BEGIN) {
        JX9_PROG = "$rc = db_begin();";
      } else if ( choice == DB_DOC_COMMIT) {	
        JX9_PROG = "$rc = db_commit();";	 
      } else if ( choice == DB_DOC_ROLLBACK) {
        JX9_PROG = "$rc = db_rollback();";
      }

      unqlite_value *value;
      int bool_result;

      if( objc != 2 ){
          Tcl_WrongNumArgs(interp, 2, objv, 0);
	  return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_PROG, strlen(JX9_PROG),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    case DB_DOC_DROP: {
      const char *JX9_DROP_COLLECTION = "$rc = db_drop_collection($argv[0]);";
      unqlite_value *value;
      int bool_result;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 1, objv, "doc_drop");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, JX9_DROP_COLLECTION, strlen(JX9_DROP_COLLECTION),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, pDb->collection_name);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      value = unqlite_vm_extract_variable(pDb->vm, "rc");
      bool_result = unqlite_value_to_bool(value);

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(bool_result));

      break;
    }

    case DB_DOC_CLOSE: {
      if (pDb->collection_name) {
        Tcl_Free(pDb->collection_name);
        pDb->collection_name = 0;
      }

      break;
    }

    /*
     * Eval a JX9 script string and get the output buffer result.
     */
    case DB_JX9_EVAL: {
      char *Jx9_script_string;
      int len;

      if( objc == 3 ){
        Jx9_script_string = Tcl_GetStringFromObj(objv[2], &len);

        if( !Jx9_script_string || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "Jx9_script_string");
        return TCL_ERROR;
      }

      result = unqlite_compile(pDb->db, Jx9_script_string, strlen(Jx9_script_string),&pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      const void *pOut = NULL;
      unsigned int nLen;
      char *iBuffer;
      Tcl_Obj *pResultStr = NULL;

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_EXTRACT_OUTPUT, &pOut, &nLen);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      if( nLen > 0 && pOut) {
        iBuffer = (char *) malloc (nLen + sizeof(char));
        sprintf(iBuffer, "%.*s", (int)nLen, (const char *)pOut);
        pResultStr = Tcl_NewStringObj(iBuffer, -1);
        free (iBuffer);
      } else if (nLen == 0) { /* Add nLen case to handle */
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

    /*
     * Eval a JX9 script file and get the output buffer result.
     */
    case DB_JX9_EVAL_FILE: {
      const char *zFile;
      Tcl_Obj *pathPtr;
      Tcl_DString translatedFilename;
      int len;

      if( objc == 3 ){
        zFile = Tcl_GetStringFromObj(objv[2], &len);

        if( !zFile || len < 1 ){
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "Jx9_script_file");
        return TCL_ERROR;
      }

      zFile = Tcl_TranslateFileName(interp, zFile, &translatedFilename);

      pathPtr = Tcl_NewStringObj(zFile, -1);
      Tcl_IncrRefCount(pathPtr);
      rc = Tcl_FSAccess(pathPtr, 4);  //To test readable (R_OK)
      Tcl_DecrRefCount(pathPtr);
      if (rc != TCL_OK) {
        return rc;
      }

      result = unqlite_compile_file(pDb->db, zFile, &pDb->vm);
      Tcl_DStringFree(&translatedFilename);

      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      result = unqlite_vm_exec(pDb->vm);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }


      const void *pOut = NULL;
      unsigned int nLen;
      char *iBuffer;
      Tcl_Obj *pResultStr = NULL;

      result = unqlite_vm_config(pDb->vm, UNQLITE_VM_CONFIG_EXTRACT_OUTPUT, &pOut, &nLen);
      if( result != UNQLITE_OK ){
        unqlite_vm_release(pDb->vm);

        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_ERROR;
      }

      if( nLen > 0 && pOut) {
        iBuffer = (char *) malloc (nLen + sizeof(char));
        sprintf(iBuffer, "%.*s", (int)nLen, (const char *)pOut);
        pResultStr = Tcl_NewStringObj(iBuffer, -1);
        free (iBuffer);
      } else if (nLen == 0) { /* Add nLen case to handle */
        Tcl_SetObjResult(interp,  Tcl_NewBooleanObj(0));
        return TCL_OK;
      }

      unqlite_vm_release(pDb->vm);
      Tcl_SetObjResult(interp,  pResultStr);

      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}



/*
**   unqlite DBNAME FILENAME ?-readonly BOOLEAN? ?-mmap BOOLEAN? ?-create BOOLEAN?
**                           ?-in-memory BOOLEAN? ?-nomutex BOOLEAN?
**
** This is the main Tcl command.  When the "unqlite" Tcl command is
** invoked, this routine runs to process that command.
**
** The first argument, DBNAME, is an arbitrary name for a new
** database connection.  This command creates a new command named
** DBNAME that is used to control that connection.  The database
** connection is deleted when the DBNAME command is deleted.
**
** The second argument is the name of the database file.
**
*/
static int DbMain(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  UnqliteDb *p;
  const char *zArg;
  int i;
  const char *zFile;
  int flags;
  Tcl_DString translatedFilename;
  int rc;


  // This extension default open flag
  flags = UNQLITE_OPEN_READWRITE | UNQLITE_OPEN_CREATE;

  if( objc==2 ){
    zArg = Tcl_GetStringFromObj(objv[1], 0);
    if( strcmp(zArg,"-version")==0 ){
      Tcl_AppendResult(interp, unqlite_lib_version(), (char*)0);
      return TCL_OK;
    } else if( strcmp(zArg,"-enable-threads")==0 ){
      int result = unqlite_lib_is_threadsafe();
      if (result)
        Tcl_AppendResult(interp, "true", (char*)0);
      else
        Tcl_AppendResult(interp, "false", (char*)0);

      return TCL_OK;
    }
  }

  for(i=3; i+1<objc; i+=2){
    zArg = Tcl_GetStringFromObj(objv[i], 0);

    if( strcmp(zArg, "-readonly")==0 ){
      int b;
      if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
      if( b ){
        flags &= ~(UNQLITE_OPEN_READWRITE|UNQLITE_OPEN_CREATE);
        flags |= UNQLITE_OPEN_READONLY;
      }else{
        flags &= ~UNQLITE_OPEN_READONLY;
        flags |= UNQLITE_OPEN_READWRITE;
      }
    }else if( strcmp(zArg, "-mmap")==0 ){
      int b;

      /*
       * UNQLITE_OPEN_MMAP: Obtain a read-only memory view of the whole database.
       */
      if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
      if( b && (flags & UNQLITE_OPEN_READONLY)==1 ){
        flags |= UNQLITE_OPEN_MMAP;
      }else{
        flags &= ~UNQLITE_OPEN_MMAP;
      }
    }else if( strcmp(zArg, "-create")==0 ){
      int b;
      if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
      if( b && (flags & UNQLITE_OPEN_READONLY)==0 ){
        flags |= UNQLITE_OPEN_CREATE;
      }else{
        flags &= ~UNQLITE_OPEN_CREATE;
      }
    }else if( strcmp(zArg, "-in-memory")==0 ){
      int b;

      /*
       * A private, in-memory database will be created.
       * The in-memory database will vanish when the database connection is closed.
       */
      if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
      if( b ){
        flags |= UNQLITE_OPEN_IN_MEMORY;
      }else{
        flags &= ~UNQLITE_OPEN_IN_MEMORY;
      }
    }else if( strcmp(zArg, "-nomutex")==0 ){
      int b;
      if( Tcl_GetBooleanFromObj(interp, objv[i+1], &b) ) return TCL_ERROR;
      if( b ){
        flags |= UNQLITE_OPEN_NOMUTEX;
      }else{
        flags &= ~UNQLITE_OPEN_NOMUTEX;
      }
    }else{
      Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
      return TCL_ERROR;
    }
  }

  if( objc<3 || (objc&1)!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv,
      "HANDLE FILENAME ?-readonly BOOLEAN? ?-mmap BOOLEAN? ?-create BOOLEAN? ?-in-memory BOOLEAN? ?-nomutex BOOLEAN? "
    );
    return TCL_ERROR;
  }

  p = (UnqliteDb *)Tcl_Alloc( sizeof(*p) );
  if( p==0 ){
    Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
    return TCL_ERROR;
  }

  memset(p, 0, sizeof(*p));
  zFile = Tcl_GetStringFromObj(objv[2], 0);
  zFile = Tcl_TranslateFileName(interp, zFile, &translatedFilename);
  rc = unqlite_open(&p->db, zFile, flags);
  Tcl_DStringFree(&translatedFilename);

  if( rc != UNQLITE_OK ) {
     unqlite_close(p->db);
     p->db = 0;
  }

  if( p->db==0 ){
    Tcl_SetResult (interp, "Create UnQlite database fail", NULL);
    Tcl_Free((char*)p);
    return TCL_ERROR;
  }

  p->interp = interp;
  zArg = Tcl_GetStringFromObj(objv[1], 0);
  Tcl_CreateObjCommand(interp, zArg, DbObjCmd, (char*)p, DbDeleteCmd);

  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Unqlite_Init --
 *
 *	Initialize the new package.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	The Unqlite package is created.
 *	One new command "unqlite" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

EXTERN int Unqlite_Init(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "unqlite", (Tcl_ObjCmdProc *) DbMain,
    	    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

#ifdef UNQLITE_ENABLE_THREADS
  // Try to enable unqlite multi-thread support
  unqlite_lib_config( UNQLITE_LIB_CONFIG_THREAD_LEVEL_MULTI );
  unqlite_lib_init();
#endif
	
    return TCL_OK;
}
