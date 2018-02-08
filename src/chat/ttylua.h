/*************************************************************************
  > File Name: /home/ljy/proj/ljy/muduo/examples/asio/chat/ttylua.h
  > Author: ljy
  > Mail: 2818880298@qq.com
  > Created Time: Fri 02 Feb 2018 10:23:09 AM CST
 ************************************************************************/

#ifndef _TTYLUA_H
#define _TTYLUA_H

// int pmain (lua_State *L);
void l_message (const char *pname, const char *msg);
int report (lua_State *L, int status);
int readStdin(lua_State *L);
int handle_luainit (lua_State *L);
int loadline (lua_State *L);

#endif // _TTYLUA_H
