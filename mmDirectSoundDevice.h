class mmDirectSoundDevice {
public:
    mmDirectSoundDevice() {}
    ~mmDirectSoundDevice() {}
    int Create(uint16_t) { return 0; }
    bool Play() { return true; }
};