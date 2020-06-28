# SnowDeformationUE4
雪和交互产生的动态变化， 利用两个RT 渲染

This uses two Render target to generate footprints in snow.

Uses first render target for displacement.
Use Pixel shader to write first render target's pixel into second one for persistent RT.
The Depth buffer will add persistent RT before computing next snow displacement.
It also does projection inside land material.

![alt text](https://github.com/tigershan1130/SnowDeformationUE4/blob/master/HighresScreenshot00000.png)

后期TODO, 动态积雪/遮挡效果:

1. 我们可以加入一个Scene Capture 在人物头顶， 往下看人物角色， 通过渲染Custom Depth Buffer 获取是否整个人或者围绕人旁边的物体要做积雪的渲染；
   通过 Compare Custom Depth, 我们可以判定所有被遮挡的Depth 全部不做积雪和雨水效果在表面。
   


