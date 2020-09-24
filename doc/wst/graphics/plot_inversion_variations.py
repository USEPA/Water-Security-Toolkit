import matplotlib.pyplot as plt

a = [0,1,2,3,3.01,5,6,7,8,9]
b = [1,1,1,1,100,100,100,100,100,100]
c = [0,1,2,2.01,3,3.01,5,6,6.01,9]
d = [2,2,2,20,20,50,50,50,150,150]
e = [0,1,2,3,4,4.01,5,5.01,6,7,8,9]
f = [3,3,3,3,3,175,175,3,3,3,3,3]

plt.plot(a,b,linewidth=2)
plt.plot(c,d,linewidth=2)
plt.plot(e,f,linewidth=2)
plt.ylim(0,200)
plt.ylabel('Injection Strength')
plt.xlabel('Time')
plt.legend(['Step','No Decrease','Pulse'])
plt.show()
