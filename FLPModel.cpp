#include "FLPModel.h"
#include <numeric>
#include <iostream>

// construct model out of Facility location problem instance
FLPModel::FLPModel(const FLPInstance &inst) {
    n_ = inst.n();
    h_ = inst.h();


    try {
		model_ = IloModel(env_);
		cplex_ = IloCplex(model_);

		//creating variables
		y_ = IloBoolVarArray(env_, h_);
		for (unsigned int i = 0; i < h_; i++){
			sprintf(var, "y(%d)", i);
			y_[i].setName(var);
			model_.add(y_[i]);
		}

		x_ = IloBoolVarMatrix(env_, n_);
		for (unsigned int i=0; i < n_; i++) {
			x_[i] = IloBoolVarArray(env_, h_);
			for(unsigned int j = 0; j < h_; ++j){

				sprintf(var, "x(%d,%d)", i, j);
				x_[i][j].setName(var);
				model_.add(x_[i][j]);
			}			
		}
		// objective function


		// constraint: all customers must be served
		for (unsigned int i = 0; i < n_; i++) {
			IloExpr expr(env_);
			for (unsigned int j = 0; j < h_; j++) {
				expr += x_[i][j];
			}
			sprintf (var, "Constraint1_%d", i);
			IloRange cons = (expr == 1);
			cons.setName(var);
			model_.add(cons);
		}

		// constraint: the capacity of a facility cannot be surpassed
		for (unsigned int j = 0; j < h_; j++) {
			IloExpr expr1(env_);
			IloExpr expr2(env_);
			for (unsigned int i = 0; i < n_; i++) {
				expr1 += inst.q(i)*x_[i][j];
			}
			expr2 = inst.Q(j)*y_[j];

			sprintf (var, "Constraint2_%d", j);
			IloRange cons = (expr1 - expr2 <= 0);
			cons.setName(var);
			model_.add(cons);
		}

	// 	// single-threaded solving
	// 	cplex_.setParam( IloCplex::Threads, 1);
	 	cplex_.setOut(env_.getNullStream());
    } catch (IloException &e) {
		cerr << "Error creating model: " << e.getMessage() << endl;
		e.end();
    }
	cplex_.exportModel("flp1.lp");
}

IloRange FLPModel::addConstraint( IloExpr leftside, int rightside){
	IloRange cons = (leftside <= rightside);
	model_.add(cons);
	return cons;
}

void FLPModel::addObjective(IloExpr Objective) {
	model_.add(IloMinimize(env_, Objective));
}

void FLPModel::removeConstraint(IloRange cons) {
	model_.remove(cons);

}

int FLPModel::solve() {
	int opt = -1;
		try {
			auto start = cplex_.getTime();
			cplex_.solve();
			auto elapsed = cplex_.getTime() - start;
			if (cplex_.getStatus() == IloAlgorithm::Optimal) {
				opt = cplex_.getObjValue();
				cout << "Solution with costs/distance of " << opt << endl;
				for (unsigned int j = 0; j < h_; j++) {
					if (cplex_.getValue(y_[j]) > 0.5) {
						cout << "\tFacility " << j << " with customers:";
						for (unsigned int i = 0; i < n_; i++) {
							if (cplex_.getValue(x_[i][j]) > 0.5) {
								cout << " " << i;
							}
						}
						cout << endl;
					}
				}
			}
			else {
				cout << "No feasible solution found" << endl;
			}
		}
		catch (IloException& e) {
			cerr << "Error solving model: " << e.getMessage() << endl;
			e.end();

		}

	 return opt;
}

