Usage:
patch [micorcode path] loopcount
Arguments:
1.[micorcode path]
path use relative path ,if path is current,[path]can Omissed
2.loopcount
loopcount=0 means: loop forever

example:
patch mike 0
1.
micorcode is locate mike directory[the currcent dirctory is Patch.eif's dirctory]
2.
0:loop forever

Log File:  test.log


Note:
Run Shell.efi first
then run Patch.efi in the Shell environment
Byo Internel Shell will hang


问题1:
操作文件大量用到了gEfiShellProtocol,所以需要调用Status = ShellInitialize();
但是Byo自带的Shell会hang掉,但它的Lib使用ShellInitialize就没有问题,以前问过Byo.没有明确答案.
先运行标准的EDKII的Shell,再在它的环境下运行Application就没有问题了.
问题2:
调用StartupAllAPs让Aps执行Function.
Function中如果有Print或时操作硬盘的动作,在调用多次后会随机assert
ASSERT z:\MdeModulePkg\Core\Dxe\Event\Tpl.c(68): OldTpl <= NewTpl
ASSERT z:\MdeModulePkg\Core\Dxe\Library\Library.c(96): Lock->Lock == EfiLockAcquired
