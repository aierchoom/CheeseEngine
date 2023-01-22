# 渲染器框架设计

## 尽可能独立且互不干扰
框架模块之间的设计应该尽可能的独立且互不干扰。
模型信息应该完全不知道图形api的细节信息，这些应由渲染器统筹管理。
假定如下几个原则
1. 模型有特定的接口提供着色器所需数据。
2. cbuffer不应该创建对应的upload resource。

## tips
1. 查询频繁更新常量缓冲区是否会有问题。
2. 着色器类拥有所有所有的ShaderInfo。
3. 假设模型是不会更改的数据。
4. Shader拥有全部的ShaderInfo。
5. Render Target。

## Model类的设计概要
1. Model由Mesh组成，Mesh包含Material/vertex/Index/数据组成？