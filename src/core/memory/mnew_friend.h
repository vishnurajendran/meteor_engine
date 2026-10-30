//
// mnew_friend.h
//
// MNEW_FRIEND lets mnew<T>() call a class's private/protected constructor,
// the same way `new T()` could from inside the class's own factory functions.
// It is part of every DEFINE_*_CLASS macro, so classes do not add it by hand.
//

#ifndef METEOR_ENGINE_MNEW_FRIEND_H
#define METEOR_ENGINE_MNEW_FRIEND_H

#define MNEW_FRIEND                                                            \
    template<typename MNewT, typename... MNewArgs>                             \
    friend MNewT* mnew(MNewArgs&&...);

#endif // METEOR_ENGINE_MNEW_FRIEND_H
