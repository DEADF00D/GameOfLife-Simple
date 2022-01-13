#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define FPS 120

struct GameOfLife{
    SDL_Surface *ScreenSurface;

    char **Screen;
    int w,h;

    SDL_Surface *Cell;
    int cellpx;

    int cells_count;

    SDL_Surface *DebugBar;

    SDL_Color WhiteColor;

    TTF_Font *font;

    int IsSimulationRunning;
    float SimulationTimeBeetween;
};

typedef struct{
    char key[SDLK_LAST];
    int mousex,mousey;
    int mousexrel,mouseyrel;
    char mousebuttons[8];
    char quit;
} Input;

typedef struct{
    unsigned int timePrev;
    unsigned int elapsed;
} L_Fps;

void L_fpsDelay(L_Fps *manager, unsigned int fps){
    manager->elapsed = SDL_GetTicks() - manager->timePrev;
    if (manager->elapsed <(1000/fps)){
        SDL_Delay((1000/fps) - (manager->elapsed));
    }
    manager->timePrev =  SDL_GetTicks();
}


void UpdateEvents(Input* in){
    SDL_Event event;
    in->mousebuttons[SDL_BUTTON_WHEELUP] = 0;
    in->mousebuttons[SDL_BUTTON_WHEELDOWN] = 0;
    while(SDL_PollEvent(&event)){
        switch (event.type){
            case SDL_KEYDOWN:
                in->key[event.key.keysym.sym]=1;
                break;
            case SDL_KEYUP:
                in->key[event.key.keysym.sym]=0;
                break;
            case SDL_MOUSEMOTION:
                in->mousex=event.motion.x;
                in->mousey=event.motion.y;
                in->mousexrel=event.motion.xrel;
                in->mouseyrel=event.motion.yrel;
                break;
            case SDL_MOUSEBUTTONDOWN:
                in->mousebuttons[event.button.button]=1;
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button!=SDL_BUTTON_WHEELUP && event.button.button!=SDL_BUTTON_WHEELDOWN)
                    in->mousebuttons[event.button.button]=0;
                break;
            case SDL_QUIT:
                in->quit = 1;
                break;
            default:
                break;
        }
    }
}

void Init_GameOfLifeScreen(char ***Screen, int w, int h){
    *Screen=(char**)calloc(h+1,sizeof(char*));
    for(int i=0;i<h;++i){
        (*Screen)[i]=(char*)calloc(w+1,sizeof(char));
    }
}

void Clear_GameOfLifeScreen(char ***Screen, int w, int h){
    for(int y=0;y<h;++y){
        memset((*Screen)[y],0,w);
    }
}

void Init_GameOfLife(struct GameOfLife *gol, int w, int h, int cellpx){
    Init_GameOfLifeScreen(&(gol->Screen), w, h);
    gol->w=w;
    gol->h=h;

    gol->ScreenSurface=SDL_SetVideoMode(cellpx*w, (cellpx*h)+32, 32, SDL_HWSURFACE);

    gol->Cell=SDL_CreateRGBSurface(SDL_HWSURFACE, cellpx, cellpx, 32, 0, 0, 0, 0);
    SDL_FillRect(gol->Cell, NULL, SDL_MapRGB(gol->ScreenSurface->format, 0, 0, 0));
    gol->cellpx=cellpx;

    gol->DebugBar=SDL_CreateRGBSurface(SDL_HWSURFACE, (gol->cellpx)*(gol->w), (gol->cellpx)*(gol->h), 32, 0, 0, 0, 0);
    SDL_FillRect(gol->DebugBar, NULL, SDL_MapRGB(gol->ScreenSurface->format, 20, 20, 20));

    gol->WhiteColor.r=255;
    gol->WhiteColor.b=255;
    gol->WhiteColor.g=255;

    gol->IsSimulationRunning=1;
    gol->SimulationTimeBeetween=0.05;
}

void Render_TheGameOfLife(struct GameOfLife *gol){
    SDL_FillRect(gol->ScreenSurface, NULL, SDL_MapRGB(gol->ScreenSurface->format, 255, 255, 255));

    SDL_Rect cur;
    for(int y=0;y<gol->h;++y){
        for(int x=0;x<gol->w;++x){
            if(gol->Screen[y][x]){
                cur.x=x*gol->cellpx;
                cur.y=y*gol->cellpx;

                SDL_BlitSurface(gol->Cell, NULL, gol->ScreenSurface, &cur);
            }
        }
    }

    cur.x=0;
    cur.y=(gol->cellpx)*(gol->h);
    SDL_BlitSurface(gol->DebugBar, NULL, gol->ScreenSurface, &cur);

    char debugTextS[512];
    memset(debugTextS,0,512);
    sprintf(debugTextS,"%dx%d (%d): Running=%d, Frame Delay=%f, Cells Size=%d, Cells count=%d",gol->w, gol->h, (gol->w)*(gol->h), gol->IsSimulationRunning, gol->SimulationTimeBeetween, gol->cellpx, gol->cells_count);

    SDL_Surface *debugText;
    debugText=TTF_RenderText_Solid(gol->font, debugTextS, gol->WhiteColor);
    cur.x=10;
    SDL_BlitSurface(debugText, NULL, gol->ScreenSurface, &cur);
    SDL_FreeSurface(debugText);
}

int CountNeighbours_TheGameOfLife(struct GameOfLife *gol, char **screen, int xx, int yy){
    int count=0;

    int xmin, xmax, ymin, ymax;
    if(xx-1<0){xmin=0;} else{xmin=xx-1;}
    if(xx+1>=gol->w){xmax=(gol->w)-1;} else{xmax=xx+1;}
    if(yy-1<0){ymin=0;} else{ymin=yy-1;}
    if(yy+1>=gol->h){ymax=(gol->h)-1;} else{ymax=yy+1;}

    for(int y=ymin;y<=ymax;++y){
        for(int x=xmin;x<=xmax;++x){
            if(screen[y][x]==1){
                if(x!=xx || y!=yy){
                    ++count;
                }
            }
        }
    }

    return count;
}

void Roll_TheGameOfLife(struct GameOfLife *gol){
    char **Screen_ToRet;
    Init_GameOfLifeScreen(&Screen_ToRet, gol->w, gol->h);

    int cc=0;
    for(int i=0;i<gol->h;++i){
        memcpy(Screen_ToRet[i],gol->Screen[i],gol->w);
        for(int p=0;p<gol->w;++p){
            if(gol->Screen[i][p]){ ++cc; }
        }
    }
    gol->cells_count=cc;

    for(int y=0;y<gol->h;++y){
        for(int x=0;x<gol->w;++x){
            int c=CountNeighbours_TheGameOfLife(gol, gol->Screen, x, y);
            if((gol->Screen)[y][x]==1){
                if(c!=2 && c!=3){
                    Screen_ToRet[y][x]=0;
                }
            }
            else if((gol->Screen)[y][x]==0){
                if(c==3){
                    Screen_ToRet[y][x]=1;
                }
            }
        }
    }

    for(int i=0;i<gol->h;++i){ free((gol->Screen)[i]); }
    free(gol->Screen);

    gol->Screen=Screen_ToRet;
}

int main(){
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    struct GameOfLife gol;
    gol.font=TTF_OpenFont("thin_pixel-7.ttf", 20);
    if(!gol.font){ printf("(error) font file not found.\n");return 1; }
    L_Fps fps;
    memset(&fps,0,sizeof(fps));

    Init_GameOfLife(&gol,300,300,6);

    SDL_WM_SetCaption("The Game of Life, by DEADF00D.", NULL);

    int SimulationSwitch_Timer=0;
    int SimulationTime_Timer=0;
    int SimulationSpeed_Timer=0;

    Input in;
    memset(&in,0,sizeof(in));
    while(!in.quit){
        UpdateEvents(&in);
        if(SimulationSwitch_Timer>=(FPS*0.1) && in.key[SDLK_SPACE]){
            gol.IsSimulationRunning=!(gol.IsSimulationRunning);
            SimulationSwitch_Timer=0;
        }else{
            ++SimulationSwitch_Timer;
        }

        if(SimulationSpeed_Timer>=(FPS*0.1)){
            if(in.key[SDLK_z]){
                gol.SimulationTimeBeetween+=0.01;
                SimulationSpeed_Timer=0;
                //printf("Speed at: %f\n",gol.SimulationTimeBeetween);
            }
            else if(in.key[SDLK_x]){
                if(gol.SimulationTimeBeetween>=0){
                    gol.SimulationTimeBeetween-=0.01;
                    SimulationSpeed_Timer=0;
                    //printf("Speed at: %f\n",gol.SimulationTimeBeetween);
                }
            }
        }else{
            ++SimulationSpeed_Timer;
        }

        if(in.key[SDLK_c]){
            Clear_GameOfLifeScreen(&(gol.Screen),gol.w, gol.h);
        }

        if(in.mousebuttons[SDL_BUTTON_LEFT]){
            if(in.mousey>0 && in.mousey<(gol.h*gol.cellpx)){
                gol.Screen[(int)in.mousey/gol.cellpx][(int)in.mousex/gol.cellpx]=1;
            }
        }
        if(in.mousebuttons[SDL_BUTTON_RIGHT]){
            if(in.mousey>0 && in.mousey<(gol.h*gol.cellpx)){
                gol.Screen[(int)in.mousey/gol.cellpx][(int)in.mousex/gol.cellpx]=0;
            }
        }

        if(gol.IsSimulationRunning && SimulationTime_Timer>=(FPS*gol.SimulationTimeBeetween)){
            Roll_TheGameOfLife(&gol);
            SimulationTime_Timer=0;
        }else{
            ++SimulationTime_Timer;
        }

        Render_TheGameOfLife(&gol);
        SDL_Flip(gol.ScreenSurface);
        L_fpsDelay(&fps, FPS);
    }

    for(int i=0;i<gol.h;++i){ free((gol.Screen)[i]); }
    free(gol.Screen);

    SDL_FreeSurface(gol.ScreenSurface);
    SDL_FreeSurface(gol.Cell);
    SDL_FreeSurface(gol.DebugBar);

    TTF_CloseFont(gol.font);

    TTF_Quit();
    SDL_Quit();
}
