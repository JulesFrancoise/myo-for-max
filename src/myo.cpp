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

#if defined(WIN_VERSION)
#define MAXAPI_USE_MSCRT
#endif

#include "ext.h"
#include "ext_obex.h"
#include "ext_systhread.h"
#include <array>
#include <deque>
#include <myo/myo.hpp>
#include <set>
#include <stdio.h>

#define atom_isnum(a) ((a)->a_type == A_LONG || (a)->a_type == A_FLOAT)
#define atom_issym(a) ((a)->a_type == A_SYM)
#define mysneg(s) ((s)->s_name)
#define atom_setvoid(p) ((p)->a_type = A_NOTHING)

typedef struct _myo t_myo;

#if defined(MAC_VERSION)
#pragma mark -
#pragma mark Myo Device Listener
#endif
class MaxMyoListener : public myo::DeviceListener {
  public:
    MaxMyoListener(t_myo *maxObject)
        : maxObject_(maxObject), num_emg_frames(0) {}

    /// Called when a paired Myo has been connected.
    void onConnect(myo::Myo *myo, uint64_t timestamp,
                   myo::FirmwareVersion firmwareVersion);

    /// Called when a paired Myo has been disconnected.
    void onDisconnect(myo::Myo *myo, uint64_t timestamp);

    /// Called when a paired Myo recognizes that it is on an arm.
    virtual void onArmSync(myo::Myo *myo, uint64_t timestamp, myo::Arm arm,
                           myo::XDirection xDirection, float rotation,
                           myo::WarmupState warmupState);

    /// Called when a paired Myo is moved or removed from the arm.
    virtual void onArmUnsync(myo::Myo *myo, uint64_t timestamp);

    // onEmgData() is called whenever a paired Myo has provided new EMG data,
    // and EMG streaming is enabled.
    void onEmgData(myo::Myo *myo, uint64_t timestamp, const int8_t *emg);

    /// Called when a paired Myo has provided new orientation data.
    void onOrientationData(myo::Myo *myo, uint64_t timestamp,
                           const myo::Quaternion<float> &rotation);

    /// Called when a paired Myo has provided new accelerometer data in units of
    /// g.
    void onAccelerometerData(myo::Myo *myo, uint64_t timestamp,
                             const myo::Vector3<float> &accel);

    /// Called when a paired Myo has provided new gyroscope data in units of
    /// deg/s.
    void onGyroscopeData(myo::Myo *myo, uint64_t timestamp,
                         const myo::Vector3<float> &gyro);

    /// Called when a paired Myo has provided a new RSSI value.
    void onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi);

    /// Called when a paired Myo receives an battery level update.
    void onBatteryLevelReceived(myo::Myo *myo, uint64_t timestamp,
                                uint8_t level);

    /// Called when a paired Myo has provided a new pose.
    void onPose(myo::Myo *myo, uint64_t timestamp, myo::Pose pose);

    // Sensor Data arrays
    uint64_t emg_timestamp;
    std::array<std::array<float, 8>, 4> emg_frames;
    std::array<float, 3> acceleration;
    std::array<float, 3> gyroscopes;
    std::array<float, 4> quaternions;

    int num_emg_frames;

    // List of connected devices
    std::set<myo::Myo *> connectedDevices;

  protected:
    // parent object structure
    t_myo *maxObject_;
};

#if defined(MAC_VERSION)
#pragma mark -
#pragma mark Max object and methods definitions
#endif
struct _myo {
    t_object self;

    MaxMyoListener *myoListener;  // Myo event listener
    myo::Myo *myoDevice;          // Current myo device (null if disconnected)
    myo::Hub *myoHub;             // Myo Hub
    t_systhread systhread;        // thread reference
    t_systhread_mutex mutex;      // mutual exclusion lock for threadsafety
    int systhread_cancel;         // thread cancel flag

    void *outlet_accel;
    void *outlet_gyro;
    void *outlet_quat;
    void *outlet_emg;
    void *outlet_poses;
    void *outlet_info;

    t_symbol *deviceName;  // Name of the Myo device
    bool myo_connect_running;
    bool listenerRunning;

    // Attributes
    int stream;
    int myoPolicy_emg;
    int myoPolicy_unlock;
    long dummy_attr_long;
};

// Method declaration
void *myo_new(t_symbol *s, long argc, t_atom *argv);
void myo_free(t_myo *self);
void myo_assist(t_myo *self, void *b, long m, long a, char *s);
void myo_info(t_myo *self);

void myo_bang(t_myo *self);
void myo_connect(t_myo *self, t_symbol *s, long argc, t_atom *argv);
void myo_disconnect(t_myo *self);
void myo_vibrate(t_myo *self, t_symbol *s, long argc, t_atom *argv);

void myo_dump_devlist(t_myo *self);
void myo_dump_emg(t_myo *self);
void myo_dump_accel(t_myo *self);
void myo_dump_gyro(t_myo *self);
void myo_dump_quat(t_myo *self);
void onMaxMyoSync(t_myo *self);

void *myo_run(t_myo *self);  // threaded function

// Attribute accessors
t_max_err myoSetStreamAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetStreamAttr(t_myo *self, t_object *attr, long *ac, t_atom **av);
t_max_err myoSetStreamEmgAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetStreamEmgAttr(t_myo *self, t_object *attr, long *ac,
                              t_atom **av);
t_max_err myoSetUnlockAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetUnlockAttr(t_myo *self, t_object *attr, long *ac, t_atom **av);
t_max_err myoSetDeviceAttr(t_myo *self, void *attr, long ac, t_atom *av);
t_max_err myoGetDeviceAttr(t_myo *self, t_object *attr, long *ac, t_atom **av);

static t_symbol *emptysym = gensym("");
static t_symbol *sym_short = gensym("short");
static t_symbol *sym_medium = gensym("medium");
static t_symbol *sym_long = gensym("long");
static t_symbol *sym_rssi = gensym("rssi");
static t_symbol *sym_battery = gensym("battery");
static t_symbol *sym_auto = gensym("auto");
static t_symbol *sym_connected = gensym("connected");
static t_symbol *sym_armsync = gensym("armsync");

t_class *myo_class;

#if defined(MAC_VERSION)
#pragma mark -
#pragma mark Functions
#endif
// main method called only once in a Max session
int C74_EXPORT main(void) {
    t_class *c;

    c = class_new("myo", (method)myo_new, (method)myo_free, sizeof(t_myo),
                  (method)NULL, A_GIMME, 0);

    class_addmethod(
        c, (method)myo_bang, "bang",
        0);  // the method it uses when it gets a bang in the left inlet
    class_addmethod(c, (method)myo_connect, "connect", A_GIMME, 0);
    class_addmethod(c, (method)myo_disconnect, "disconnect", 0);
    class_addmethod(c, (method)myo_info, "info", 0);
    class_addmethod(c, (method)myo_dump_devlist, "devices", 0);
    class_addmethod(
        c, (method)myo_assist, "assist", A_CANT,
        0);  // (optional) assistance method needs to be declared like this
    class_addmethod(c, (method)myo_vibrate, "vibrate", A_GIMME, 0);

    // Stream
    // ------------------------------
    CLASS_ATTR_LONG(c, "stream", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN(c, "stream", 0);
    CLASS_ATTR_FILTER_MAX(c, "stream", 1);
    CLASS_ATTR_ACCESSORS(c, "stream", (method)myoGetStreamAttr,
                         (method)myoSetStreamAttr);
    CLASS_ATTR_STYLE_LABEL(c, "stream", 0, "onoff", "Enable/Disable Playing");

    // Stream EMGs
    // ------------------------------
    CLASS_ATTR_LONG(c, "emg", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN(c, "emg", 0);
    CLASS_ATTR_FILTER_MAX(c, "emg", 1);
    CLASS_ATTR_ACCESSORS(c, "emg", (method)myoGetStreamEmgAttr,
                         (method)myoSetStreamEmgAttr);
    CLASS_ATTR_STYLE_LABEL(c, "emg", 0, "onoff",
                           "Enable/Disable EMG Streaming");

    // Device name
    // ------------------------------
    CLASS_ATTR_SYM(c, "device", 0, t_myo, deviceName);
    CLASS_ATTR_STYLE_LABEL(c, "device", 0, "auto", "Name of the myo device");
    CLASS_ATTR_ACCESSORS(c, "device", (method)myoGetDeviceAttr,
                         (method)myoSetDeviceAttr);

    // Keep unlocked
    // ------------------------------
    CLASS_ATTR_LONG(c, "unlock", 0, t_myo, dummy_attr_long);
    CLASS_ATTR_FILTER_MIN(c, "unlock", 0);
    CLASS_ATTR_FILTER_MAX(c, "unlock", 1);
    CLASS_ATTR_ACCESSORS(c, "unlock", (method)myoGetUnlockAttr,
                         (method)myoSetUnlockAttr);
    CLASS_ATTR_STYLE_LABEL(c, "unlock", 0, "onoff", "Keep Myo Unlocked");

    class_register(CLASS_BOX, c);
    myo_class = c;

    return 0;
}

/**
 * Constructor
 */
void *myo_new(t_symbol *s, long argc, t_atom *argv) {
    t_myo *self;

    self = (t_myo *)object_alloc(myo_class);

    long ac = attr_args_offset((short)argc, argv);

    if (self) {
        systhread_mutex_new(&self->mutex, 0);

        self->outlet_info = outlet_new(self, NULL);
        self->outlet_poses = outlet_new(self, NULL);
        self->outlet_emg = outlet_new(self, NULL);
        self->outlet_quat = outlet_new(self, NULL);
        self->outlet_gyro = outlet_new(self, NULL);
        self->outlet_accel = outlet_new(self, NULL);

        self->myoListener = NULL;
        self->myoDevice = NULL;
        self->systhread = NULL;
        self->stream = false;
        self->myoPolicy_emg = true;
        self->myoPolicy_unlock = false;

        self->deviceName = sym_auto;

        self->listenerRunning = false;

        // get device name if object has argument
        if (ac > 0 && atom_issym(argv)) self->deviceName = atom_getsym(argv);

        try {
            // First, we create a Hub with our application identifier. Be sure
            // not to use the com.example namespace when
            // publishing your application. The Hub provides access to one or
            // more Myos.
            self->myoHub = new myo::Hub("com.julesfrancoise.maxmyo");

            // Create a device listener and add is to the hub listeners
            self->myoListener = new MaxMyoListener(self);
            self->myoHub->addListener(self->myoListener);
            self->myo_connect_running = true;
        } catch (const std::exception &e) {
            object_error((t_object *)self, e.what());
            self->myo_connect_running = false;
        }

        // process attributes
        attr_args_process(self, (short)argc, argv);
    }
    return (self);
}

/**
 * Destructor
 */
void myo_free(t_myo *self) {
    myo_disconnect(self);
    self->myoDevice = NULL;

    if (self->myoListener) {
        self->myoHub->removeListener(self->myoListener);
        delete self->myoListener;
    }

    if (self->myoHub) {
        self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyStandard);
        delete self->myoHub;
    }

    if (self->mutex) systhread_mutex_free(self->mutex);
}

/**
 * [info]
 * get information on available devices + battery/RSSI of the device in use
 */
void myo_info(t_myo *self) {
    if (!self->myo_connect_running) return;
    // myo_dump_devlist(self);
    if (self->myoDevice) {
        self->myoDevice->requestBatteryLevel();
        self->myoDevice->requestRssi();
    }
}

/**
 * dumps the list of available devices
 */
void myo_dump_devlist(t_myo *self) {
    if (!self->myo_connect_running) return;
    long listlen = (long)(1 + self->myoListener->connectedDevices.size());
    t_atom *devlist = (t_atom *)sysmem_newptr(sizeof(t_atom) * listlen);
    atom_setsym(devlist, gensym("devices"));
    int offset = 1;
    for (auto device : self->myoListener->connectedDevices) {
        atom_setsym(devlist + offset, gensym(device->getName().c_str()));
        offset++;
    }
    outlet_list(self->outlet_info, NULL, (short)listlen, devlist);
}

/**
 * inlet/outlet hover info
 */
void myo_assist(t_myo *self, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET) {  // inlet
        sprintf(s, "bang, connect, disconnect, stream <1/0>");
    } else {  // outlet
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

/**
 * Threaded function to run the event listener
 */
void *myo_run(t_myo *self) {
    if (!self->myo_connect_running) return NULL;
    // We catch any exceptions that might occur below -- see the catch statement
    // for more details.
    try {
        self->systhread_cancel = false;

        // Finally we enter our main loop.
        while (1) {
            // test if we're being asked to die, and if so return before we do
            // the work
            if (self->systhread_cancel) break;

            systhread_mutex_lock(self->mutex);

            // In each iteration of our main loop, we run the Myo event loop for
            // a set number of milliseconds.
            // In this case, we wish to update our display 50 times a second, so
            // we run for 1000/20 milliseconds.
            self->myoHub->run(20);

            systhread_mutex_unlock(self->mutex);
        }

        self->listenerRunning = false;
        self->systhread_cancel = false;
        systhread_exit(0);  // this can return a value to systhread_join();
        return NULL;
    } catch (const std::exception &e) {
        self->listenerRunning = false;
        object_error((t_object *)self, e.what());
    }

    return NULL;
}

/**
 * [bang]
 * outputs the current frame
 */
void myo_bang(t_myo *self) {
    if (!self->myo_connect_running) return;
    if (self->myoDevice) {
        myo_dump_emg(self);
        myo_dump_quat(self);
        myo_dump_gyro(self);
        myo_dump_accel(self);
    }
}

/**
 * dumps emg data
 */
void myo_dump_emg(t_myo *self) {
    t_atom value_out[8];
    for (int j = 0; j < 8; j++) {
        atom_setfloat(
            value_out + j,
            self->myoListener
                ->emg_frames[self->myoListener->num_emg_frames - 1][j]);
    }
    if (self->myoListener->num_emg_frames > 1) {
        self->myoListener->num_emg_frames--;
    }
    outlet_list(self->outlet_emg, NULL, 8, value_out);
}

/**
 * dumps acceleration data
 */
void myo_dump_accel(t_myo *self) {
    t_atom value_out[3];
    for (int j = 0; j < 3; j++) {
        atom_setfloat(value_out + j, self->myoListener->acceleration[j]);
    }
    outlet_list(self->outlet_accel, NULL, 3, value_out);
}

/**
 * dumps gyro data
 */
void myo_dump_gyro(t_myo *self) {
    t_atom value_out[3];
    for (int j = 0; j < 3; j++) {
        atom_setfloat(value_out + j, self->myoListener->gyroscopes[j]);
    }
    outlet_list(self->outlet_gyro, NULL, 3, value_out);
}

/**
 * dumps orientation data (quaternions)
 */
void myo_dump_quat(t_myo *self) {
    t_atom value_out[4];
    for (int j = 0; j < 4; j++) {
        atom_setfloat(value_out + j, self->myoListener->quaternions[j]);
    }
    outlet_list(self->outlet_quat, NULL, 4, value_out);
}

/**
 * [connect]
 * starts the listener thread to connect to available myos
 */
void myo_connect(t_myo *self, t_symbol *s, long argc, t_atom *argv) {
    if (self->listenerRunning || !self->myo_connect_running) return;
    if (self->systhread == NULL) {
        self->listenerRunning = true;
        systhread_create((method)myo_run, self, 0, 0, 0, &self->systhread);
    }
}

/**
 * [disconnect]
 * disconnect from sensors (stops listener thread)
 */
void myo_disconnect(t_myo *self) {
    if (!self->myo_connect_running) return;
    unsigned int ret;
    if (self->systhread) {
        // object_post((t_object *)self, "stopping thread");
        self->systhread_cancel = true;          // tell the thread to stop
        systhread_join(self->systhread, &ret);  // wait for the thread to stop
        self->systhread = NULL;
        self->listenerRunning = false;
    }
}

/**
 * [vibrate]
 * trigger myo vibrations
 */
void myo_vibrate(t_myo *self, t_symbol *s, long argc, t_atom *argv) {
    if (!self->myoDevice || !self->myo_connect_running) return;
    if (argc == 0) {
        self->myoDevice->notifyUserAction();
    } else {
        if (atom_isnum(argv)) {
            switch (atom_getlong(argv)) {
                case 0:
                    self->myoDevice->vibrate(myo::Myo::vibrationShort);
                    break;

                case 1:
                    self->myoDevice->vibrate(myo::Myo::vibrationMedium);
                    break;

                case 2:
                    self->myoDevice->vibrate(myo::Myo::vibrationLong);
                    break;

                default:
                    break;
            }
        } else if (atom_issym(argv)) {
            t_symbol *arg_sym = atom_getsym(argv);
            if (arg_sym == sym_short)
                self->myoDevice->vibrate(myo::Myo::vibrationShort);
            else if (arg_sym == sym_medium)
                self->myoDevice->vibrate(myo::Myo::vibrationMedium);
            else if (arg_sym == sym_long)
                self->myoDevice->vibrate(myo::Myo::vibrationLong);
            else
                return;
        }
    }
}

#if defined(MAC_VERSION)
#pragma mark -
#pragma mark Attributes
#endif
/**
 * [stream 0/1]
 * set streaming attribute (sensor data streamed as received)
 */
t_max_err myoSetStreamAttr(t_myo *self, void *attr, long ac, t_atom *av) {
    if (ac > 0 && atom_isnum(av)) {
        self->stream = atom_getlong(av) != 0;
    } else
        object_error((t_object *)self,
                     "missing or invalid arguments for stream");

    return MAX_ERR_NONE;
}

/**
 * get streaming attribute (sensor data streamed as received)
 */
t_max_err myoGetStreamAttr(t_myo *self, t_object *attr, long *ac, t_atom **av) {
    if ((*ac) == 0 || (*av) == NULL) {
        // otherwise allocate memory
        *ac = 1;

        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac)))) {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }

    atom_setlong(*av, self->stream);
    return MAX_ERR_NONE;
}

/**
 * [emg 0/1]
 * specifies if the raw emg data should be streamed
 */
t_max_err myoSetStreamEmgAttr(t_myo *self, void *attr, long ac, t_atom *av) {
    if (ac > 0 && atom_isnum(av)) {
        self->myoPolicy_emg = atom_getlong(av) != 0;
        if (!self->myo_connect_running) return MAX_ERR_NONE;
        if (self->myoDevice) {
            if (self->myoPolicy_emg)
                self->myoDevice->setStreamEmg(myo::Myo::streamEmgEnabled);
            else
                self->myoDevice->setStreamEmg(myo::Myo::streamEmgDisabled);
        }
    } else
        object_error((t_object *)self, "missing or invalid arguments for emg");

    return MAX_ERR_NONE;
}

/**
 * specifies if the raw emg data should be streamed
 */
t_max_err myoGetStreamEmgAttr(t_myo *self, t_object *attr, long *ac,
                              t_atom **av) {
    if ((*ac) == 0 || (*av) == NULL) {
        // otherwise allocate memory
        *ac = 1;

        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac)))) {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }

    atom_setlong(*av, self->myoPolicy_emg);
    return MAX_ERR_NONE;
}

/**
 * [unlock 0/1]
 * specifies if the Myo is unlocked for pose recognition
 */
t_max_err myoSetUnlockAttr(t_myo *self, void *attr, long ac, t_atom *av) {
    if (ac > 0 && atom_isnum(av)) {
        self->myoPolicy_unlock = atom_getlong(av) != 0;
        if (!self->myo_connect_running) return MAX_ERR_NONE;
        if (self->myoPolicy_unlock && self->myo_connect_running)
            self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyNone);
        else
            self->myoHub->setLockingPolicy(myo::Hub::lockingPolicyStandard);
    } else
        object_error((t_object *)self,
                     "missing or invalid arguments for unlock");

    return MAX_ERR_NONE;
}

/**
 * specifies if the Myo is unlocked for pose recognition
 */
t_max_err myoGetUnlockAttr(t_myo *self, t_object *attr, long *ac, t_atom **av) {
    if ((*ac) == 0 || (*av) == NULL) {
        // otherwise allocate memory
        *ac = 1;

        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac)))) {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }

    atom_setlong(*av, self->myoPolicy_unlock);
    return MAX_ERR_NONE;
}

/**
 * [device <myoname>]
 * specifies the name of the myo device to listen
 */
t_max_err myoSetDeviceAttr(t_myo *self, void *attr, long ac, t_atom *av) {
    if (ac > 0 && atom_issym(av)) {
        if (self->deviceName != atom_getsym(av)) {
            self->deviceName = atom_getsym(av);
            self->myoDevice = NULL;
            if (!self->myo_connect_running) return MAX_ERR_NONE;
            if (self->deviceName == sym_auto) {
                if (self->myoListener->connectedDevices.size() > 0)
                    self->myoDevice =
                        *(self->myoListener->connectedDevices.begin());
            } else {
                for (auto device : self->myoListener->connectedDevices) {
                    if (std::string(self->deviceName->s_name) ==
                        device->getName()) {
                        self->myoDevice = device;
                    }
                }
            }
            onMaxMyoSync(self);
            if (!self->myoDevice) {
                object_warn(
                    (t_object *)self,
                    ("Myo named " + std::string(self->deviceName->s_name) +
                     " is not connected. Waiting...")
                        .c_str());
            }
        }
    } else {
        object_error((t_object *)self,
                     "missing or invalid arguments for device");
    }

    return MAX_ERR_NONE;
}

/**
 * specifies the name of the myo device to listen
 */
t_max_err myoGetDeviceAttr(t_myo *self, t_object *attr, long *ac, t_atom **av) {
    if ((*ac) == 0 || (*av) == NULL) {
        // otherwise allocate memory
        *ac = 1;

        if (!(*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac)))) {
            *ac = 0;
            return MAX_ERR_OUT_OF_MEM;
        }
    }

    atom_setsym(*av, self->deviceName);
    return MAX_ERR_NONE;
}

/**
 * outputs max messages when max is synced with a myo
 */
void onMaxMyoSync(t_myo *self) {
    if (!self->myo_connect_running) return;
    t_atom deviceInfo[2];
    atom_setsym(deviceInfo, sym_connected);
    if (self->myoDevice) {
        if (self->myoPolicy_emg)
            self->myoDevice->setStreamEmg(myo::Myo::streamEmgEnabled);
        else
            self->myoDevice->setStreamEmg(myo::Myo::streamEmgDisabled);
        atom_setsym(deviceInfo + 1, gensym(self->myoDevice->getName().c_str()));
        object_post((t_object *)self,
                    ("Connected to myo " + self->myoDevice->getName()).c_str());
    } else {
        atom_setlong(deviceInfo + 1, 0);
    }
    outlet_list(self->outlet_info, NULL, 2, deviceInfo);
}

#if defined(MAC_VERSION)
#pragma mark -
#pragma mark Myo Device Listener: Methods
#endif
void MaxMyoListener::onConnect(myo::Myo *myo, uint64_t timestamp,
                               myo::FirmwareVersion firmwareVersion) {
    connectedDevices.insert(myo);

    if (maxObject_->deviceName == sym_auto) {
        if (connectedDevices.size() == 1) {
            maxObject_->myoDevice = myo;
        }
    } else {
        if (std::string(maxObject_->deviceName->s_name) == myo->getName()) {
            maxObject_->myoDevice = myo;
        }
    }
    for (int i = 0; i < 4; i++) emg_frames[i].fill(0.);
    onMaxMyoSync(maxObject_);
    myo_dump_devlist(maxObject_);
}

void MaxMyoListener::onDisconnect(myo::Myo *myo, uint64_t timestamp) {
    connectedDevices.erase(myo);
    if (maxObject_->myoDevice == myo) {
        object_post((t_object *)maxObject_,
                    ("Disconnected from myo " + myo->getName()).c_str());
        maxObject_->myoDevice = NULL;
        if (connectedDevices.size() > 0 && maxObject_->deviceName == sym_auto) {
            maxObject_->myoDevice = *(connectedDevices.begin());
        }
    }
    onMaxMyoSync(maxObject_);
    for (int i = 0; i < 4; i++) emg_frames[i].fill(0.);
    acceleration.fill(0);
    gyroscopes.fill(0);
    quaternions.fill(0);
}

void MaxMyoListener::onArmSync(myo::Myo *myo, uint64_t timestamp, myo::Arm arm,
                               myo::XDirection xDirection, float rotation,
                               myo::WarmupState warmupState) {
    if (!maxObject_->myo_connect_running) return;
    if (myo != maxObject_->myoDevice) return;
    t_atom arm_sync[6];
    atom_setsym(arm_sync, sym_armsync);
    atom_setlong(arm_sync + 1, 1);
    if (arm == myo::Arm::armLeft) {
        atom_setsym(arm_sync + 2, gensym("Left"));
    } else if (arm == myo::Arm::armRight) {
        atom_setsym(arm_sync + 2, gensym("Right"));
    } else {
        atom_setsym(arm_sync + 2, gensym("Unknown"));
    }
    if (xDirection == myo::XDirection::xDirectionTowardWrist) {
        atom_setsym(arm_sync + 3, gensym("TowardWrist"));
    } else if (xDirection == myo::XDirection::xDirectionTowardElbow) {
        atom_setsym(arm_sync + 3, gensym("TowardElbow"));
    } else {
        atom_setsym(arm_sync + 3, gensym("Unknown"));
    }
    atom_setfloat(arm_sync + 4, rotation);
    if (warmupState == myo::WarmupState::warmupStateCold) {
        atom_setsym(arm_sync + 5, gensym("Cold"));
    } else if (warmupState == myo::WarmupState::warmupStateWarm) {
        atom_setsym(arm_sync + 5, gensym("Warm"));
    } else {
        atom_setsym(arm_sync + 5, gensym("Unknown"));
    }
    outlet_list(maxObject_->outlet_info, NULL, 6, arm_sync);
}

void MaxMyoListener::onArmUnsync(myo::Myo *myo, uint64_t timestamp) {
    t_atom arm_sync[2];
    atom_setsym(arm_sync, sym_armsync);
    atom_setlong(arm_sync + 1, 0);
    outlet_list(maxObject_->outlet_info, NULL, 2, arm_sync);
}

void MaxMyoListener::onAccelerometerData(myo::Myo *myo, uint64_t timestamp,
                                         const myo::Vector3<float> &accel) {
    if (myo != maxObject_->myoDevice) return;
    acceleration[0] = accel.x();
    acceleration[1] = accel.y();
    acceleration[2] = accel.z();
    if (maxObject_->stream) myo_dump_accel(maxObject_);
}

void MaxMyoListener::onGyroscopeData(myo::Myo *myo, uint64_t timestamp,
                                     const myo::Vector3<float> &gyro) {
    if (myo != maxObject_->myoDevice) return;
    gyroscopes[0] = gyro.x();
    gyroscopes[1] = gyro.y();
    gyroscopes[2] = gyro.z();
    if (maxObject_->stream) myo_dump_gyro(maxObject_);
}

void MaxMyoListener::onOrientationData(myo::Myo *myo, uint64_t timestamp,
                                       const myo::Quaternion<float> &rotation) {
    if (myo != maxObject_->myoDevice) return;
    quaternions[0] = rotation.x();
    quaternions[1] = rotation.y();
    quaternions[2] = rotation.z();
    quaternions[3] = rotation.w();
    if (maxObject_->stream) myo_dump_quat(maxObject_);
}

void MaxMyoListener::onEmgData(myo::Myo *myo, uint64_t timestamp,
                               const int8_t *emg) {
    if (myo != maxObject_->myoDevice || num_emg_frames == 4) return;
    if (maxObject_->stream || emg_timestamp != timestamp) {
        num_emg_frames = 0;
    }
    emg_timestamp = timestamp;
    for (int i = 0; i < 8; i++) {
        emg_frames[num_emg_frames][i] = static_cast<float>(emg[i]) / (float)127.;
    }
    num_emg_frames++;
    if (maxObject_->stream) myo_dump_emg(maxObject_);
}

void MaxMyoListener::onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi) {
    if (myo != maxObject_->myoDevice) return;
    t_atom value_out[2];
    atom_setsym(value_out, sym_rssi);
    atom_setlong(value_out + 1, static_cast<int>(rssi));
    outlet_list(maxObject_->outlet_info, NULL, 2, value_out);
}

void MaxMyoListener::onBatteryLevelReceived(myo::Myo *myo, uint64_t timestamp,
                                            uint8_t level) {
    if (myo != maxObject_->myoDevice) return;
    t_atom value_out[2];
    atom_setsym(value_out, sym_battery);
    atom_setlong(value_out + 1, static_cast<int>(level));
    outlet_list(maxObject_->outlet_info, NULL, 2, value_out);
}

void MaxMyoListener::onPose(myo::Myo *myo, uint64_t timestamp, myo::Pose pose) {
    if (myo != maxObject_->myoDevice) return;
    t_atom value_out[1];
    atom_setsym(value_out, gensym(pose.toString().c_str()));
    outlet_list(maxObject_->outlet_poses, NULL, 1, value_out);
}
