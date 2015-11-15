/**
 *
 * @file myo.cpp
 * @author Jules Françoise <jfrancoi@sfu.ca>
 *
 * @brief Max interface object for the Myo Armband
 *
 * Copyright (C) 2015 by Jules Françoise.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <myo/myo.hpp>
#include "ext.h"
#include "ext_obex.h"
#include <stdio.h>
#include "ext_systhread.h"
#include <array>

#define atom_isnum(a) ((a)->a_type == A_LONG || (a)->a_type == A_FLOAT)
#define atom_issym(a) ((a)->a_type == A_SYM)
#define mysneg(s) ((s)->s_name)
#define atom_setvoid(p) ((p)->a_type = A_NOTHING)

typedef struct _myo t_myo;

#pragma mark -
#pragma mark Myo Device Listener
class MaxMyoListener : public myo::DeviceListener {
public:
    MaxMyoListener(t_myo *maxObject)
    : emgSamples(), maxObject_(maxObject)
    {
    }
    
    // onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
    void onUnpair(myo::Myo* myo, uint64_t timestamp)
    {
        // We've lost a Myo.
        // Let's clean up some leftover state.
        emgSamples.fill(0);
        acceleration.fill(0);
        gyroscopes.fill(0);
        quaternions.fill(0);
    }
    
    // onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
    void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg);
    
    /// Called when a paired Myo has provided new orientation data.
    void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& rotation);
    
    /// Called when a paired Myo has provided new accelerometer data in units of g.
    void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel);
    
    /// Called when a paired Myo has provided new gyroscope data in units of deg/s.
    void onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro);
    
    /// Called when a paired Myo has provided a new RSSI value.
    void onRssi(myo::Myo* myo, uint64_t timestamp, int8_t rssi);
    
    /// Called when a paired Myo receives an battery level update.
    void onBatteryLevelReceived(myo::Myo* myo, uint64_t timestamp, uint8_t level);
    
    /// Called when a paired Myo has provided a new pose.
    void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose);
    
    
    // The values of this array is set by onEmgData() above.
    std::array<float, 8> emgSamples;
    std::array<float, 3> acceleration;
    std::array<float, 3> gyroscopes;
    std::array<float, 4> quaternions;
    
protected:
    t_myo *maxObject_;
};

#pragma mark -
#pragma mark Max object and methods definitions
struct _myo {
    t_object self;
    
    MaxMyoListener     *myoListener;
    myo::Myo           *myoDevice;
    myo::Hub           *myoHub;
    t_systhread         systhread;			// thread reference
    t_systhread_mutex	mutex;				// mutual exclusion lock for threadsafety
    int                 systhread_cancel;	// thread cancel flag
    
    void                *outlet_accel;
    void                *outlet_gyro;
    void                *outlet_quat;
    void                *outlet_emg;
    void                *outlet_poses;
    void                *outlet_info;
    
    bool myo_connected;
    int stream;
    int myoPolicy_emg;
    int myoPolicy_unlock;
    long dummy_attr_long;
};


void myo_bang(t_myo *self);
void myo_dump_emg(t_myo *self);
void myo_dump_accel(t_myo *self);
void myo_dump_gyro(t_myo *self);
void myo_dump_quat(t_myo *self);
void *myo_get(t_myo *self);  // threaded function
void myo_connect(t_myo *self, t_symbol *s, long argc, t_atom *argv);
void myo_disconnect(t_myo *self);
void myo_info(t_myo *self);
void myo_assist(t_myo *self, void *b, long m, long a, char *s);
void *myo_new(t_symbol *s, long argc, t_atom *argv);
void myo_free(t_myo *self);

t_max_err myoSetPlayAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetPlayAttr(t_myo *self, t_object *attr, long* ac, t_atom** av);
t_max_err myoSetStreamEmgAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetStreamEmgAttr(t_myo *self, t_object *attr, long* ac, t_atom** av);
t_max_err myoSetUnlockAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetUnlockAttr(t_myo *self, t_object *attr, long* ac, t_atom** av);

t_class *myo_class;

#pragma mark -
#pragma mark Functions

// main method called only once in a Max session
int C74_EXPORT main(void)
{
    t_class *c;
    
    c = class_new("myo",
                  (method)myo_new,
                  (method)myo_free,
                  sizeof(t_myo),
                  (method)NULL,
                  A_GIMME,
                  0);
    
    class_addmethod(c, (method)myo_bang,	   "bang",		0);	// the method it uses when it gets a bang in the left inlet
    class_addmethod(c, (method)myo_connect,    "connect",    A_GIMME, 0);
    class_addmethod(c, (method)myo_disconnect, "disconnect",     0);
    class_addmethod(c, (method)myo_info,       "info",     0);
    class_addmethod(c, (method)myo_assist,     "assist",   A_CANT, 0);	// (optional) assistance method needs to be declared like this
    
    // Play
    // ------------------------------
    CLASS_ATTR_LONG(c, "stream", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN (c, "stream", 0);
    CLASS_ATTR_FILTER_MAX (c, "stream", 1);
    CLASS_ATTR_ACCESSORS  (c, "stream", (method)myoGetPlayAttr, (method)myoSetPlayAttr);
    CLASS_ATTR_STYLE_LABEL(c, "stream", 0, "onoff", "Enable/Disable Playing");
    
    // Stream EMGs
    // ------------------------------
    CLASS_ATTR_LONG(c, "emg", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN (c, "emg", 0);
    CLASS_ATTR_FILTER_MAX (c, "emg", 1);
    CLASS_ATTR_ACCESSORS  (c, "emg", (method)myoGetStreamEmgAttr, (method)myoSetStreamEmgAttr);
    CLASS_ATTR_STYLE_LABEL(c, "emg", 0, "onoff", "Enable/Disable EMG Streaming");
    
    // Keep unlocked
    // ------------------------------
    CLASS_ATTR_LONG(c, "unlock", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN (c, "unlock", 0);
    CLASS_ATTR_FILTER_MAX (c, "unlock", 1);
    CLASS_ATTR_ACCESSORS  (c, "unlock", (method)myoGetUnlockAttr, (method)myoSetUnlockAttr);
    CLASS_ATTR_STYLE_LABEL(c, "unlock", 0, "onoff", "Keep Myo Unlocked");
    
    class_register(CLASS_BOX, c);
    myo_class = c;
    
    return 0;
}

void *myo_new(t_symbol *s, long argc, t_atom *argv)
{
    t_myo *self;
    
    self = (t_myo *)object_alloc(myo_class);
    
    // int ac = attr_args_offset(argc, argv);
    
    if (self) {
        systhread_mutex_new(&self->mutex,0);
        
        self->outlet_info  = outlet_new(self, NULL);
        self->outlet_poses = outlet_new(self, NULL);
        self->outlet_emg   = outlet_new(self, NULL);
        self->outlet_quat  = outlet_new(self, NULL);
        self->outlet_gyro  = outlet_new(self, NULL);
        self->outlet_accel = outlet_new(self, NULL);
        
        self->myoListener = NULL;
        self->myoDevice = NULL;
        self->systhread = NULL;
        self->stream = false;
        self->myoPolicy_emg = true;
        self->myoPolicy_unlock = false;
        self->myo_connected = false;
        
        try {
            // First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
            // publishing your application. The Hub provides access to one or more Myos.
            self->myoHub = new myo::Hub("com.julesfrancoise.maxmyo");
        } catch (const std::exception& e) {
            error(e.what());
        }
        
        attr_args_process(self, argc, argv);
    }
    return(self);
}

void myo_free(t_myo *self)
{
    // stop thread
    myo_disconnect(self);
    
    // free out mutex
    if (self->mutex)
        systhread_mutex_free(self->mutex);
    
    if (self->myoListener)
        delete self->myoListener;
    if (self->myoHub)
        delete self->myoHub;
}

void myo_info(t_myo *self)
{
    if (self->myoDevice) {
        self->myoDevice->requestBatteryLevel();
        self->myoDevice->requestRssi();
    } else {
        post("No Myo armband connected");
    }
}

void myo_assist(t_myo *self, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { //inlet
        sprintf(s, "bang, connect, disconnect, stream <1/0>");
    }
    else {	// outlet
        switch (a) {
            case 0:
                sprintf(s, "Accelerometer data (3D)");
                break;
            case 1:
                sprintf(s, "Gyroscope data (3D)");
                break;
            case 2:
                sprintf(s, "Orientation data (Quaternions: 4D)");
                break;
            case 3:
                sprintf(s, "EMG data");
                break;
            case 4:
                sprintf(s, "Poses (from native Myo SDK)");
                break;
            case 5:
                sprintf(s, "info");
                break;
            default:
                break;
        }
    }
}

void *myo_get(t_myo *self)
{
    // We catch any exceptions that might occur below -- see the catch statement for more details.
    try {
        post("Attempting to find a Myo...");
        
        // Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
        // immediately.
        // waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
        // if that fails, the function will return a null pointer.
        self->myoDevice = self->myoHub->waitForMyo(10000);
        
        // If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
        if (!self->myoDevice) {
            throw std::runtime_error("Unable to find a Myo!");
        }
        
        // Next we enable EMG streaming on the found Myo.
        if (self->myoPolicy_emg)
            self->myoDevice->setStreamEmg(myo::Myo::streamEmgEnabled);
        else
            self->myoDevice->setStreamEmg(myo::Myo::streamEmgDisabled);
        
        if (self->myoPolicy_unlock) {
            self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyNone);
        }
        
        self->myoListener = new MaxMyoListener(self);
        
        // Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
        // Hub::run() to send events to all registered device listeners.
        self->myoHub->addListener(self->myoListener);
        
        // We've found a Myo.
        self->myo_connected = true;
        self->systhread_cancel = false;
        post("Connected to a Myo armband!");
        
        // Finally we enter our main loop.
        while (1)
        {
            // test if we're being asked to die, and if so return before we do the work
            if (self->systhread_cancel)
                break;
            
            systhread_mutex_lock(self->mutex);
            
            // In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
            // In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
            self->myoHub->run(20);
            
            systhread_mutex_unlock(self->mutex);
        }
        
        self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyStandard);
        post("Disconnected from the Myo armband!");
        self->myo_connected = false;
        self->myoDevice = NULL;
        self->systhread_cancel = false;			// reset cancel flag for next time, in case
        // the thread is created again
        
        systhread_exit(0);						// this can return a value to systhread_join();
        return NULL;
    } catch (const std::exception& e) {
        error(e.what());
        self->myo_connected = false;
    }
    
    return NULL;
}

void myo_bang(t_myo *self)
{
    if (self->myo_connected) {
        myo_dump_emg(self);
        myo_dump_quat(self);
        myo_dump_gyro(self);
        myo_dump_accel(self);
        
    }
}

void myo_dump_emg(t_myo *self)
{
    t_atom value_out[8];
    for(int j = 0; j < 8; j++)
    {
        atom_setfloat(value_out+j, self->myoListener->emgSamples[j]);
    }
    outlet_list(self->outlet_emg, NULL, 8, value_out);
}

void myo_dump_accel(t_myo *self)
{
    t_atom value_out[3];
    for(int j = 0; j < 3; j++)
    {
        atom_setfloat(value_out+j, self->myoListener->acceleration[j]);
    }
    outlet_list(self->outlet_accel, NULL, 3, value_out);
}


void myo_dump_gyro(t_myo *self)
{
    t_atom value_out[3];
    for(int j = 0; j < 3; j++)
    {
        atom_setfloat(value_out+j, self->myoListener->gyroscopes[j]);
    }
    outlet_list(self->outlet_gyro, NULL, 3, value_out);
}


void myo_dump_quat(t_myo *self)
{
    t_atom value_out[4];
    for(int j = 0; j < 4; j++)
    {
        atom_setfloat(value_out+j, self->myoListener->quaternions[j]);
    }
    outlet_list(self->outlet_quat, NULL, 4, value_out);
}

void myo_connect(t_myo *self, t_symbol *s, long argc, t_atom *argv)
{
    if(self->myo_connected)
    {
        post("Myo armband already connected");
        return;
    }
    
    if (self->systhread == NULL)
    {
        systhread_create((method) myo_get, self, 0, 0, 0, &self->systhread);
    }
}

void myo_disconnect(t_myo *self)
{
    unsigned int ret;
    
    if (self->systhread)
    {
        //post("stopping thread");
        self->systhread_cancel = true;			// tell the thread to stop
        systhread_join(self->systhread, &ret);	// wait for the thread to stop
        self->systhread = NULL;
    }
}

#pragma mark -
#pragma mark Attributes
t_max_err myoSetPlayAttr(t_myo *self, void *attr, long ac, t_atom *av)
{
    if(ac > 0 && atom_isnum(av)) {
        self->stream = atom_getlong(av) != 0;
    }
    else
        object_error((t_object *)self, "missing or invalid arguments for stream");
    
    return MAX_ERR_NONE;
    
}

t_max_err myoGetPlayAttr(t_myo *self, t_object *attr, long* ac, t_atom** av)
{
    if ((*ac) == 0 || (*av) == NULL)
    {
        //otherwise allocate memory
        *ac = 1;
        
        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac))))
        {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }
    
    atom_setlong(*av, self->stream);
    return MAX_ERR_NONE;
}

t_max_err myoSetStreamEmgAttr(t_myo *self, void *attr, long ac, t_atom *av)
{
    if(ac > 0 && atom_isnum(av)) {
        self->myoPolicy_emg = atom_getlong(av) != 0;
        if (self->myo_connected) {
            if (self->myoPolicy_emg)
                self->myoDevice->setStreamEmg(myo::Myo::streamEmgEnabled);
            else
                self->myoDevice->setStreamEmg(myo::Myo::streamEmgDisabled);
        }
    }
    else
        object_error((t_object *)self, "missing or invalid arguments for stream");
    
    return MAX_ERR_NONE;
    
}

t_max_err myoGetStreamEmgAttr(t_myo *self, t_object *attr, long* ac, t_atom** av)
{
    if ((*ac) == 0 || (*av) == NULL)
    {
        //otherwise allocate memory
        *ac = 1;
        
        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac))))
        {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }
    
    atom_setlong(*av, self->myoPolicy_emg);
    return MAX_ERR_NONE;
}

t_max_err myoSetUnlockAttr(t_myo *self, void *attr, long ac, t_atom *av)
{
    if(ac > 0 && atom_isnum(av)) {
        self->myoPolicy_unlock = atom_getlong(av) != 0;
        if (self->myo_connected) {
            if (self->myoPolicy_unlock)
                self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyNone);
            else
                self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyStandard);
        }
    }
    else
        object_error((t_object *)self, "missing or invalid arguments for stream");
    
    return MAX_ERR_NONE;
    
}

t_max_err myoGetUnlockAttr(t_myo *self, t_object *attr, long* ac, t_atom** av)
{
    if ((*ac) == 0 || (*av) == NULL)
    {
        //otherwise allocate memory
        *ac = 1;
        
        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac))))
        {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }
    
    atom_setlong(*av, self->myoPolicy_unlock);
    return MAX_ERR_NONE;
}

#pragma mark -
#pragma mark Myo Device Listener: Methods
void MaxMyoListener::onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel)
{
    acceleration[0] = accel.x();
    acceleration[1] = accel.y();
    acceleration[2] = accel.z();
    if (maxObject_->stream)
        myo_dump_accel(maxObject_);
}

void MaxMyoListener::onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro)
{
    gyroscopes[0] = gyro.x();
    gyroscopes[1] = gyro.y();
    gyroscopes[2] = gyro.z();
    if (maxObject_->stream)
        myo_dump_gyro(maxObject_);
}

void MaxMyoListener::onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& rotation)
{
    quaternions[0] = rotation.x();
    quaternions[1] = rotation.y();
    quaternions[2] = rotation.z();
    quaternions[3] = rotation.w();
    if (maxObject_->stream)
        myo_dump_quat(maxObject_);
}

void MaxMyoListener::onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
{
    for (int i = 0; i < 8; i++) {
        emgSamples[i] = static_cast<float>(emg[i]) / 127.;
    }
    if (maxObject_->stream)
        myo_dump_emg(maxObject_);
}

void MaxMyoListener::onRssi(myo::Myo* myo, uint64_t timestamp, int8_t rssi)
{
    t_atom value_out[2];
    atom_setsym(value_out, gensym("rssi"));
    atom_setlong(value_out+1, static_cast<int>(rssi));
    outlet_list(maxObject_->outlet_info, NULL, 2, value_out);
}

void MaxMyoListener::onBatteryLevelReceived(myo::Myo* myo, uint64_t timestamp, uint8_t level)
{
    t_atom value_out[2];
    atom_setsym(value_out, gensym("battery"));
    atom_setlong(value_out+1, static_cast<int>(level));
    outlet_list(maxObject_->outlet_info, NULL, 2, value_out);
}

void MaxMyoListener::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
{
    t_atom value_out[1];
    atom_setsym(value_out, gensym(pose.toString().c_str()));
    outlet_list(maxObject_->outlet_poses, NULL, 1, value_out);
}
