add_executable(chat_client client.cc)
target_link_libraries(chat_client muduo_net)

add_executable(chat_server server.cc ttylua.cpp)
target_link_libraries(chat_server muduo_net)
target_link_libraries(chat_server lua)
target_link_libraries(chat_server dl)

# add_executable(chat_server_threaded server_threaded.cc)
# target_link_libraries(chat_server_threaded muduo_net)

# add_executable(chat_server_threaded_efficient server_threaded_efficient.cc)
# target_link_libraries(chat_server_threaded_efficient muduo_net)

# add_executable(chat_server_threaded_highperformance server_threaded_highperformance.cc)
# target_link_libraries(chat_server_threaded_highperformance muduo_net)

