* ind.va test
* First load module: "devload ind"

l1 1 0 ind l=10pH
c1 1 0 10p ic=1

.model ind ind(level=2)

.control
tran 1p 100p uic
plot v(1)
.endc
