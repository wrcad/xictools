* cap.va test
* First load module: "devload cap"

l1 1 0 10pH ic=1ma
c1 1 0 cap c=10p

.model cap cap(level=2)

.control
tran 1p 100p uic
plot v(1)
.endc
