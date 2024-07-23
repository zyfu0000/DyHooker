//
//  ViewController.m
//  HookTest
//
//  Created by ZhiyangFu on 2024/7/20.
//

#import "ViewController.h"

#include <HookFramework/HookFramework.h>
#include <pthread.h>
#include "DyHooker.hpp"

@interface ViewController ()

@end

@implementation ViewController

+ (void)load {
//    int8_t argTypes[4];
//    argTypes[0] = DYH_TYPE_POINTER;
//    argTypes[1] = DYH_TYPE_POINTER;
//    argTypes[2] = DYH_TYPE_POINTER;
//    argTypes[3] = DYH_TYPE_POINTER;
//    hook_func("usr/lib/libSystem.dylib", "pthread_create", DYH_TYPE_INT, argTypes, 4);
    
    int8_t argTypes[2];
    argTypes[0] = DYH_TYPE_INT;
    argTypes[1] = DYH_TYPE_INT;
//    hook_func("HookFramework.framework/HookFramework", "hook_c_func", DYH_TYPE_INT, argTypes, 2);
    hook_func("HookFramework.framework/HookFramework", "_Z11hook_c_funcii", DYH_TYPE_INT, argTypes, 2);
}

#define N 5

static void *run(void *arg) {
    size_t job = *(size_t*)arg;
    printf("Job %zu\n", job);
    return NULL;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
//    size_t jobs[N];
//    pthread_t threads[N];
//    for (size_t i=0; i<N; ++i) {
//        jobs[i] = i;
//        pthread_create(threads+i, NULL, run, jobs+i);
//    }
    
    hook_c_func(1, 2);
}


@end
