#
# Simple prototype of door sentinel
#
# Monitor sensors
# Activate porch light
# Unlock solenoid latch-plate
#
function startup {iosetup;run getpir;run lights;run lockdog};
function getpir {p=d4|d5; d13=p; snooze(100)};
function unlock {d=10};
function lock {d=0};
function lights {if (p) {d3=1; snooze(30000)} else {d3=0; snooze(250)}};
function iosetup {pinmode(2,1);pinmode(3,1);pinmode(13,1);d2=0;d3=0;p=0;d=0};
function lockdog {d2=d; if (d) d=d-1;d2=d;snooze(1000)};
