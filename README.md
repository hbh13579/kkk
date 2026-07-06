# kkk 包管理器
一款简易的linux包管理器，通过git仓库快速安装卸载程序。
## 一,kkk的安装
### 方法1：手动安装
1,点击release  
2,点击kkk  
3,打开终端  
4,移到kkk所在目录
```bash
cd “kkk所在目录
```
5,安装kkk
```bash
sudo cp ./kkk /usr/local/bin
```
6,基础配置
```bash
mkdir -p ~/.config/kkk
mkdir -p ~/.config/kkk/kkkremove
mkdir -p ~/.config/kkk/tmp_kkk_pkg
touch ~/.config/kkk/kkk.conf
echo "test = https://github.com/hbh13579/test.git" > ~/.config/kkk/kkk.conf
```
7,测试kkk
```bash
sudo kkk -i test
```
正常输出**ok**
```bash
sudo kkk -r test
```
正常输出**okr**
%% 如果测试没问题的话， 手动安装就完成了 。 %%
### 方式2：脚本安装
脚本安装 **【正在开发】** 
