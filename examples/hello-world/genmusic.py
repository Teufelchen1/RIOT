import math


t = [x for x in range(0, 101)]

out = [math.floor(math.sin(y*2*math.pi/100)*127)+128 for y in t]
out = [math.floor(math.sin(y*2*math.pi/50)*127)+128 for y in t]
out = [255 if y > 50 else 0 for y in t]


def offset(freq, wait):
	offset = (1/freq) - (1/500)
	target = (1/500) - wait - offset
	print(target, offset)

offset(365, 0.00004)
#print(out)

out = [int(math.sin(2*math.pi*x*400/8000)*127)+128 for x in range(0, 1000)]
print(out)