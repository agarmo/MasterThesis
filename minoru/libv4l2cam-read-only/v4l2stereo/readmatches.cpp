#include <iostream>
#include <fstream>

#include <sys/stat.h>

#include <cstdlib>



struct MatchData {
    unsigned short int probability;
    unsigned short int x;
    unsigned short int y;
    unsigned short int disparity;
};

struct MatchDataColour {
    unsigned short int probability;
    unsigned short int x;
    unsigned short int y;
    unsigned short int disparity;
    unsigned char r, g, b;
    unsigned char pack;
};



int main(){

    struct stat results;

    if(stat("test.bin", &results) == 0)
        std::cout << results.st_size << std::endl;
    else{
        std::cout << "error getting the size of the file" << std::endl;
        return -1;
    }

    // opening the file
    std::ifstream filestream; 
    filestream.open("test.bin", std::ios::in | std::ios::binary);

    if( filestream == NULL){
        std::cout << "Erorr, unable to open test.bin" << std::endl;
        return -1;
    }

    // ready to read the file
    
    int length = results.st_size/sizeof(MatchDataColour);

    std::cout << "lengde "  << length << std::endl;

    MatchDataColour *m = new MatchDataColour[length]; // read the 100 first entries
    

    for (int i = 0; i < length ; i++){
    
        filestream.read(reinterpret_cast<char*> (&m[i]), sizeof(MatchDataColour) );

/*        
        m[i].probability = atoi(buffer);

        filestream.read(buffer, sizeof(short int) );
        m[i].x = atoi(buffer);

        filestream.read(buffer, sizeof(short int) );
        m[i].y = atoi(buffer);

        filestream.read(buffer, sizeof(short int) );
        m[i].disparity = atoi(buffer);

        filestream.read(buffer, sizeof(char));
        m[i].r = *buffer;
        
        filestream.read(buffer, sizeof(char));
        m[i].g = *buffer;

        filestream.read(buffer, sizeof(char));
        m[i].b =* buffer;

        filestream.read(buffer, sizeof(char));
        m[i].pack = *buffer;
*/

    }


   for (int i = 0; i < length; i++){

        using namespace std;

        cout << "Element nr " << i << endl;
        cout << "Probability " << m[i].probability << "\t x position " << m[i].x << " \t y position " << m[i].y << " \t Disparity " << m[i].disparity;
        cout << "\t Red Green Blue" << (m[i].r) << " " << (m[i].g) << " " << m[i].b << " \t Pack " << m[i].pack << endl;
   }
       

    



    filestream.close();

    delete[] m;

    return 0;
    



}



