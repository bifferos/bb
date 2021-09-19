These sub-directories contain profiles that can be used to generate 
specific firmware.  Each profile consists of kernel and busybox 
config, rootfs and some docs.  By running the build shell ('build' 
command in the parent dir) with profile as argument you can use the 
'busy' and 'kernel' commands to tweak your firmware, additionally 
you can change the rootfs files in the relevant rootfs directory.
When happy run 'make' and your firmware will be built.

Any changes to busybox or kernel profile you make in the shell will
get persisted out to the relevant profile directory and not 'polute'
the master kernel build directory.

