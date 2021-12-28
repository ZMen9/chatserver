# 定义SRC_LIST变量，包含server目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./database DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
# 指定生成可执行文件ChatServer
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
# 可执行文件ChatServer链接时需要的三个库文件muduo_net muduo_base pthread
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread hiredis)
