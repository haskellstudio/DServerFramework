##分布式服务器框架

##框架简介
* 此框架由三种服务器组成：
	* n个`连接服务器`：负责处理客户端的网络链接，并转发客户端和逻辑服务器之间的消息。
	* n个`逻辑服务器`：负责处理(由链接服务器转发过来的)客户端的网络消息
	* 1个`全局服务器`：负责处理全局逻辑
	* 其实还额外需要一个神器：[etcd](https://github.com/coreos/etcd)，作为服务器框架中的`逻辑服务器`感知`链接服务器`的新增和失效
	* 无论拓扑图请参考[架构图](https://github.com/IronsDu/DServerFramework/blob/master/%E6%9C%8D%E5%8A%A1%E5%99%A8%E6%9E%B6%E6%9E%84%E6%96%87%E6%A1%A3/%E6%9C%8D%E5%8A%A1%E5%99%A8%E6%95%B4%E4%BD%93%E6%9E%B6%E6%9E%84.png)

*  框架内置组件:
	*  多连接ssdb客户端
	*  redis/ssdb代理服务器
	*  仅支持C++语言的RPC(支持protobuf类型消息，适合服务器之间业务开发，并支持异步回调)

##既定规则(建议)
    *  当客户端链接某`链接服务器`会(自动)分配一个Primary类型的`逻辑服务器`(以后默认由后者负责处理此客户端的所有消息)(且不能重设)

    *  客户端还拥有一个Slave类型的`逻辑服务器`,可由任意`逻辑服务器`（告诉`链接服务器`)进行设置（当设置了其Slave逻辑服务器时，则由Slave逻辑服务器处理客户端的后续消息，直到取消为止)

    *  当然，在客户端的Primary的`逻辑服务器`中，也可以设置此服务器作为此客户端的Slave（也就是说客户端的Primary和Slave为同一个`逻辑服务器`,这并没什么冲突）
    
    *  实际项目中，您可以借助第三方负载均衡服务器来分配`链接服务器`的地址给客户端.
    *  内置的`ssdb`客户端和`ssdb`代理服务器，如果你的应用需要数据库，不妨一试。

##使用方式
* 开发应用时，只需要关注`客户端`和`逻辑服务器`之间的消息设定，不需要考虑`链接服务器`，且没有任何(需要引入的)预设消息。
* 开发应用时，只需要分别实现`逻辑服务器`中的`initLogicServerExt`函数 和 (可选)`全局服务器`中的`initCenterServerExt`函数  （通常进行消息handle注册即可）。
* 作为示例
    * `中心服务器`的扩展参考[CenterServerExt.cpp](https://github.com/IronsDu/DServerFramework/blob/master/DDServerFramework/src/test/CenterServerExt.cpp)
    * `逻辑服务器`的扩展参考[LogicServerExt.cpp](https://github.com/IronsDu/DServerFramework/blob/master/DDServerFramework/src/test/LogicServerExt.cpp)
    * 测试客户端请看：[SimulateClient.cpp](https://github.com/IronsDu/DServerFramework/blob/master/DDServerFramework/src/SimulatePlayer/SimulateClient.cpp#L34)
