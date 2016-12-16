
NETWORK_STACK_COMPACT_x64.Fv
  use open source UDK code revision 19732 (20160122) to build a network stack FV.
  Source: http://192.168.6.5/svn/unc/Platform/VIA/ZX100/PreBuild/UDKNetWork_20160122.zip

NETWORK_STACK_COMPACT_x64_all.Fv
  Base on NETWORK_STACK_COMPACT_x64.Fv
    1. DEFINE HTTP_BOOT_ENABLE = TRUE
    2. add NetworkPkg/IpSecDxe/IpSecDxe.inf
    
    