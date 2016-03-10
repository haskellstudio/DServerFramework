ProxyConfig = {
	backends = {
		{id = 0, ip = "127.0.0.1", port = "8888"}
	},

	listenPort = 8555,			--对外（提供代理功能）的监听端口
	sharding_function = "test_sharding"	-- 指定sharding函数，其根据key返回对应的backend id
}

function test_sharding(key)
	return 0
end