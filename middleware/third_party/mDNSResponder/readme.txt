Brief:          This module is the implementation of Apple Bonjour (only supported by a multicast DNS (mDNS) server).
Usage:          GCC: Include the module with "include $(SOURCE_DIR)/middleware/third_party/mDNSResponder/module.mk" in your GCC project Makefile.
                KEIL: Drag the middleware/third_party/mDNSResponder folder to your project. Add middleware/third_party/mDNSResponder/include to include paths.
                IAR: Drag the middleware/third_party/mDNSResponder folder to your project. Add middleware/third_party/mDNSResponder/include to include paths.
Dependency:     Please also include the LwIP module, and make sure that LWIP_UDP/LWIP_IPV6/LWIP_IPV6_FRAG/LWIP_IGMP/LWIP_AUTOIP is enabled.
Notice:         None.
Relative doc:   Please refer to the open source user guide under the doc folder for more detail.
Example project:Please find the project under the project folder with the mdns_ prefix.

