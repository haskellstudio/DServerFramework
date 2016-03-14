##分布式服务器框架

##它有哪些功能？

* 提供多个`链接服务器`和`逻辑服务器`
    *  `链接服务器`负责处理客户端的链接和通信
    *  `逻辑服务器`负责处理(由`链接服务器`转发来的)客户端的消息请求
    *  当客户端链接某`链接服务器`会(自动)分配一个Primary类型的`逻辑服务器`(以后默认由后者负责处理客户端消息)(且不能重设)
    *  客户端还拥有一个Slave类型的`逻辑服务器`,可由任意`逻辑服务器`进行设置（当具有此服务器时，则由它处理客户端消息)
* 提供一个`全局服务器`:负责处理`内部服务器`之间的消息(当然理论上可由它做一些全局逻辑)
* 开发应用时，客户端和`逻辑服务器`之间的消息设定，不需要考虑`链接服务器`，且没有任何(需要引入的)预设消息。
* 开发引用时，只需要分别实现`逻辑服务器`中的initLogicServerExt函数 和 `全局服务器`中的initCenterServerExt函数  （通常进行消息handle注册即可）。
* demo
    * `中心服务器`的扩展参考[CenterServerExt.cpp](https://github.com/IronsDu/DServerFramework/blob/master/DDServerFramework/src/test/CenterServerExt.cpp)
    * `逻辑服务器`的扩展参考[LogicServerExt.cpp](https://github.com/IronsDu/DServerFramework/blob/master/DDServerFramework/src/test/LogicServerExt.cpp)
