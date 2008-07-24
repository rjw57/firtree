/*
 * Selective logging - Lua binding.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog_lua.c,v 1.10 2008/04/08 17:30:56 fanf2 Exp $
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "selog.h"

LUALIB_API int luaopen_selog(lua_State *L);

/**********************************************************************/

static struct selog_selector *
self(lua_State *L) {
	return(luaL_checkudata(L, 1, "selog"));
}

static int
sel_call(lua_State *L) {
	struct selog_selector *sel = self(L);
	selog_buffer buf;
	lua_Debug ar;

	if(!selog_on(sel))
		return(0);
	lua_concat(L, lua_gettop(L) - 1);
	selog_bufinit(buf, sel);
	if(selog_level(sel) == SELOG_TRACE &&
	    lua_getstack(L, 1, &ar) &&
	    lua_getinfo(L, "lnS", &ar))
		selog_add(buf, "%s:%d %s() ",
		    ar.source[0] == '@' ? ar.source+1 : ar.short_src,
		    ar.currentline, ar.name ? ar.name : "");
	selog_add(buf, "%s %s: ", selog_name(sel), selog_level(sel));
	selog_add(buf, "%s", lua_tostring(L, -1));
	selog_write(buf);
	return(0);
}

static int
sel_index(lua_State *L) {
	struct selog_selector *sel = self(L);
	const char *str = lua_tostring(L, 2);
	if(str != NULL)
		switch(str[0]) {
		case('e'):
			if(strcmp(str, "exitval") != 0)
				break;
			if(SELOG_LEVEL(sel) != SELOG_EXIT)
				break;
			lua_pushinteger(L, SELOG_EXITVAL(sel));
			return(1);
		case('l'):
			if(strcmp(str, "level") != 0)
				break;
			lua_pushstring(L, selog_level(sel));
			return(1);
		case('n'):
			if(strcmp(str, "name") != 0)
				break;
			lua_pushstring(L, selog_name(sel));
			return(1);
		case('o'):
			if(strcmp(str, "on") != 0)
				break;
			lua_pushboolean(L, selog_on(sel));
			return(1);
		}
	lua_pushnil(L);
	return(1);
}

/* create a selector in a Lua userdata object */
static int
mod_new(lua_State *L) {
	struct selog_selector *sel;
	const char *name;
	const char *str;
	unsigned int level;

	name = luaL_checkstring(L, 1);
	str = luaL_checkstring(L, 2);
	level = selog_parse_level(str, strlen(str));
	if(level == SELOG_NOLEVEL)
		return(luaL_argerror(L, 2,
		    lua_pushfstring(L, "invalid level "LUA_QS, str)));
	if(level == SELOG_FATAL)
		level = SELOG_FATAL(luaL_optint(L, 3, 1));
	sel = lua_newuserdata(L, sizeof(*sel));
	memset(sel, 0, sizeof(*sel));
	sel->name = name;
	sel->level = level;
	luaL_getmetatable(L, "selog");
	/* save the name so that Lua doesn't treat it as garbage */
	lua_getfield(L, -1, "names");  /* local t = mt.names */
	lua_pushvalue(L, 1);           /* k = name */
	lua_pushboolean(L, 1);         /* v = true */
	lua_settable(L, -3);           /* t[k] = v */
	lua_pop(L, 1);                 /* pop t */
	/* setmetatable(sel, mt) */
	lua_setmetatable(L, -2);
	return(1);
}

static int
mod_call(lua_State *L) {
	/* selog(...) is equivalent to selog.new(selog, ...) so we
	need to remove the first argument before we pass the call on */
	lua_remove(L, 1);
	return(mod_new(L));
}

/* initialize selog */
static int
mod_open(lua_State *L) {
	const char *config = luaL_checkstring(L, 1);
	const char **spelling = NULL;
	const char *word;
	size_t n, i;
	lua_settop(L, 2);
	/* is there a spelling argument? */
	if(!lua_isnoneornil(L, 2)) {
		luaL_checktype(L, 2, LUA_TTABLE);
		n = lua_objlen(L, 2) + 1;
		spelling = calloc(n, sizeof(*spelling));
		if(spelling == NULL)
			goto error;
		for(i = 1; i < n; i++) {
			/* word = spelling[i] */
			lua_pushinteger(L, i);
			lua_gettable(L, 2);
			word = lua_tostring(L, -1);
			if(word == NULL)
				return(luaL_argerror(L, 2,
				    "non-string in spelling list"));
			spelling[i-1] = word;
			/* pop word */
			lua_pop(L, 1);
		}
		spelling[i-1] = NULL;
	}
	if(selog_open(config, spelling) >= 0)
		return(0);
error:
	return(luaL_error(L, "selog.open() failed: %s", strerror(errno)));
}

/* initialize binding */

static const luaL_Reg functions[] = {
	{ "__call", mod_call },
	{ "new",    mod_new  },
	{ "open",   mod_open },
	{ NULL, NULL }
};

static const luaL_Reg methods[] = {
	{ "__call",  sel_call  },
	{ "__index", sel_index },
	{ NULL, NULL }
};

LUALIB_API int
luaopen_selog(lua_State *L) {
	/* Create metatable for selectors */
	luaL_newmetatable(L, "selog");
	/* sel_mt.names = {} */
	lua_newtable(L);
	lua_setfield(L, -2, "names");
	/* Populate sel_mt with methods */
	luaL_register(L, NULL, methods);
	/* Create and populate the module's main table */
	luaL_register(L, "selog", functions);
	lua_pushvalue(L, -1);
	lua_setmetatable(L, -2);
	/* return module */
	return(1);
}

/* eof */
