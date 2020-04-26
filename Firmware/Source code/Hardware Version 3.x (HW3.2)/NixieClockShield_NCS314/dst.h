#ifndef __DST_H
#define __DST_H
/**
 * Algorithms to determine whether a date is in daylight saving time.
 * Subclasses determine behavior such as never, EU1996, US2007.
 * David Levine <levined@ieee.org> 20200405
 */

class DSTnever;
class DSTeu1996;
class DSTus2007;

class DST
{
  public:
    enum Mode {DST_NEVER, DST_EU1996, DST_US2007, DST_DEFAULT = DST_NEVER};

    /**
     * Sets the static methods to refer to the specified Mode, see enum Mode.
     */
    static void setDSTMode(unsigned int mode);

    /**
     * tmElements_t is defined in TimeLib.h
     * offset is hours from UTC
     * wasDST is true if the time had already reflected DST
     */
    static bool isDST(tmElements_t &, int hoursOffset, bool wasDST);

    virtual ~DST();
    virtual bool isDST_i(tmElements_t &, int hoursOffset, bool wasDST) const = 0;
  private:
    static const DSTnever dstNever;
    static const DSTeu1996 dstEU1996;
    static const DSTus2007 dstUS2007;
    static const DST *dst;
};


class DSTnever : public DST
{
  public:
    virtual bool isDST_i(tmElements_t &, int hoursOffset, bool wasDST) const;
};


class DSTeu1996 : public DST
{
  public:
    virtual bool isDST_i(tmElements_t &, int hoursOffset, bool wasDST) const;
};


class DSTus2007 : public DST
{
  public:
    virtual bool isDST_i(tmElements_t &, int hoursOffset, bool wasDST) const;
};
#endif
