# WavePackets

### 2021 春夏计算机图形学研究进展

## 运行与配置

项目需要使用 Visual Studio 2019 进行编译，并且安装 Windows SDK 8.1。

使用 Visual Studio 2019 打开 `Demo.sln` 并编译即可运行。

## 参考的开源资料

- [microsoft/DXUT框架](https://github.com/Microsoft/DXUT)
- [microsoft/FX11框架](https://github.com/Microsoft/FX11)
- [论文作者提供的部分代码](https://github.com/jeschke/water-wave-packets/tree/v1.0.0)
- [CImg.h](https://cimg.eu/)

# 实现的部分

我们参考了论文作者部分的代码逻辑和渲染框架，以及参考了作者提供的一系列水波模拟中的自定义物理参数的数值，我们所撰写的代码位于 `Demo` 目录。

以下列出自己实现的部分：

- `maths.h` & `maths_impl.h` ：实现了论文中的大部分数学公式和计算，与其他模块分离
- `Packet.h` & `PacketVertex.h` & `packets_utils.h`: 我们重写了类的定义与实现，通过 `pool.h` 优化了类的内存管理
- `map.h`: 优化了地形图的访问方式
- `GameObject.h` & `Geometry.h`: 引入了新的场景对象管理方式以及地形几何顶点数据的生成方法
- `Renderer.cpp`: 我们重新设计了场景的渲染方式与顶点数据的生成传输
- `shaders.fx`：优化了场景物体、水面、地形的渲染着色器
