from scipy.optimize import minimize
from scipy.optimize import least_squares
import numpy as np

# tdoa means 'time difference of arrival'
def tdoa_multilateration(x0, stations, tdoa, c):
	#def error(x, r, t, c):
	#	return (np.linalg.norm(x - r[0]) + sum([-np.linalg.norm(x - r[i]) + c*(t[i]-t[0]) for i in range(1,len(r))]))
	#def eval_solution(x, r_0, r_i, t_0, t_i, c):
	#	return (
	#			np.linalg.norm(x - r_0)
	#		-	np.linalg.norm(x - r_i)
	#		+	c*(t_i-t_0)
	#	)
	def error(x, r_0, r_i, t_0, t_i, c):
		#return np.linalg.norm(x0-r_0) - np.array([np.linalg.norm(x0-r_i[i]) for i in range (len(r_i))]) + np.array([c*(t_i[i]-t_0) for i in range(len(t_i))])
		return ( 
				np.linalg.norm(x-r_0)
		  -		[np.linalg.norm(x-r_i[i]) for i in range (len(r_i))]
		  +		[c*(t_i[i]-t_0) for i in range(len(t_i))]
		)
	def constraint(x):
		return (np.linalg.norm(x)-20.)


	#bnd = ((-100,-100,15.),(100,100,np.Infinity))
	bnd = (-np.inf,np.inf)
	con = {'type': 'ineq', 'fun':constraint}

	#print(error(x0,stations[0], stations[1:], tdoa[0], tdoa[1:], c))
	#print('---------------------------------------------------------------')
	#print(eval_solution(x0,stations[0], stations[1:], tdoa[0], tdoa[1:], c))
	#print('---------------------------------------------------------------')
	res = least_squares(error, x0, args=(stations[0], stations[1:], tdoa[0], tdoa[1:], c)) #, ftol=1e-15, gtol=1e-15, xtol=1e-15
	#res = minimize(error, x0, args=(stations[0], stations[1:], tdoa[0], tdoa[1:], c), method='Nelder-Mead', bounds=bnd, constraints=con, options={'disp':True,'adaptive':True,'xatol':1e-12, 'fatol':1e-12})
	return res

if __name__ == "__main__":
	c = 3e5 # speed of light [km/s]
	#xz = np.array([0.,10.,0.])
	#stations = np.array([[0.,0.,0.], [0.,2.,0.], [0.,4.,0.], [0.,6.,0.], [0.,8.,0.], [0.,10.,0.]])
	#tdoa = np.array([0.,.1980390272/c,.7703296143/c,1.6619037897/c,2.8062484749/c,4.1421356237/c])
	xz = np.array([0.,0.,20.])
	stations = np.array([[0.,0.,0.],[5.,5.,0.],[10.,-5.,0.],[15.,0.,0.],[20.,-5.,0.]])
	tdoa = np.array([0., 4.04401e-6, 9.70959e-6, 1.66667e-5, 2.9076e-5])
	
	x0 = [0.,0.,1.]
	res =  tdoa_multilateration(x0, stations, tdoa, c)
	print(res)
	print('Actual emitter location:		', xz)
	print('Calculated emitter locaion:	', res.x)
	print('Distance Error:				',np.linalg.norm(res.x-xz))
