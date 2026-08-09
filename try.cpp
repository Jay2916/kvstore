#include <iostream>


int main(){
    char c;
    while(std::cin.get(c)){
        if(c == ' ')
            std::cout << "space" << std::endl; 
        else if(c == '\n')
            std::cout << "new line" << std::endl; 
        else
            std::cout << c << std::endl;
                
    }
}