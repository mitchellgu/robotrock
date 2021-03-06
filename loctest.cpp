#include <iostream>
#include "wallfollower.cpp"
#include "localize.cpp"

int running=1;
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("closing PWM nicely\n");
        running = 0;
    }
};

int main(){
	signal(SIGINT, sig_handler);
	Motor* left = new Motor(0,2,4,false);
	Motor* right = new Motor(4,6,2,true);
	Location* location = new Location(0.0,0.0,0.0);
	IR* irf = new IR(0);
	IR* irr = new IR(1);
	IR* irlf = new IR(3);
	IR* irlb = new IR(2);
	mraa::Gpio* uirb = new mraa::Gpio(8);
	Wallfollower* wf= new Wallfollower(left,right,irf,irr,irlf,irlb,uirb,location);
	Localize* loc=new Localize("example.txt",location);
	int channel=1,mode=1,pm=1;
	bool localized = false,hitWall=false;
	sleep(1);
	while(running&&!localized) {
		pm=mode;
		channel = wf->run_follower(channel);
		mode=wf->locating_channel();
		//std::cout<<"Returned channel "<<mode<<std::endl;
		if(mode!=pm)
		{
			if(pm==3&&!hitWall)
			{
				loc->wallFound(irlf->getDistance());
				hitWall=true;
				//sleep(3);
			}
			else if(pm==3||mode==4||mode==5)
			{
				localized=loc->atCorner(irf->getDistance(),mode);
			}
		}
	} 
	left->stop();
	right->stop();
	sleep(1);	
	return 0;
}

