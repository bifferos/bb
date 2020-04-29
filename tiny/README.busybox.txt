Busybox
=======

Busybox has gotten a few extra applets:

mixer - When using a USB sound device it's nice to set the level.  This is
using OSS, make sure support for that is in the kernel.

wavplay - Convenient way of playing simple sounds on the Bifferboard via an 
attached USB audio converter.

button - Daemon to monitor a GPIO button and do something when it's pressed.

buttondsr - Daemon to monitor a USB serial converter dsr line and do
something when it changes level.  This is a really cheap way of adding a
button without breaking into the Bifferboard case (using only the USB port),
as these USB->rs232 converters sell for pennies.

