//
// Created by ssj5v on 05-10-2024.
//

#ifndef IDRAWCALLSUBMITABLE_H
#define IDRAWCALLSUBMITABLE_H

class IMeteorDrawable {
    public:
    virtual void prepareForDraw()=0;
    virtual void raiseDrawCall()=0;
};

#endif //IDRAWCALLSUBMITABLE_H
