# DirHTTP

一个轻量级的 Windows 目录 HTTP 服务器，支持右键菜单快速启动，自动打开浏览器访问。

## 一、功能特性

- **快速启动**：通过右键菜单或直接命令行启动 HTTP 服务
- **自动打开浏览器**：服务启动后自动在默认浏览器中打开访问地址
- **多网络接口支持**：显示所有可用的局域网访问地址
- **中文路径支持**：完整支持 UTF-8 编码的中文文件和目录名
- **目录浏览**：提供友好的文件列表界面，区分文件夹和文件
- **MIME 类型自动识别**：支持常见的 HTML、CSS、JS、图片、视频、音频等文件类型
- **安全路径检查**：防止目录遍历攻击，确保访问路径在根目录范围内
- **端口自动分配**：自动绑定可用端口，避免端口冲突

## 二、安装
先下载这个压缩包然后解压

![image.png](https://lingview.xyz/file/22627ba0aa33455392cd1315927b4155)

![image.png](https://lingview.xyz/file/c2873a041c5b46edb85fa82acc2f2f60)

创建环境变量（推荐）

![image.png](https://lingview.xyz/file/d027976753f94db28eeb2d3a3de478ea)

![image.png](https://lingview.xyz/file/ab22dfb01f414ba5a70268a483877191)

然后使用管理员权限打开cmd

![image.png](https://lingview.xyz/file/3608227f7c2344bf8ee2ba21f49aecab)

然后输入`dirhttp`回车可以查看支持的命令参数

![image.png](https://lingview.xyz/file/072932c32b9a4c2db6a5cd599c84d06c)
使用`dirhttp --install`将程序注册到鼠标右键菜单
```bash
dirhttp --install
```

![image.png](https://lingview.xyz/file/30fc522e5ad14d1499a63da327ee2d6d)

## 三、使用
显示安装完成后即可再右键菜单见到（win11可能会在二级菜单）
![image.png](https://lingview.xyz/file/82adb67354d4471e994759e7cb44fb1f)

点击开启后会自动使用空闲端口启动http服务并且再浏览器自动打开
![image.png](https://lingview.xyz/file/3f57ac7d2945415584337c9ab08ac837)

![image.png](https://lingview.xyz/file/6eebdd145a1043c8abe8f2945dd432a5)

