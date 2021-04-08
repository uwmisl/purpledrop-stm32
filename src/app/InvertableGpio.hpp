/** Wrapper for Gpio to allow for runtime inversion
 */
template < class Pin >
class InvertableGpio : public Pin
{
public:
	using Output = GpioInverted<typename Pin::Output>;
	using Input = GpioInverted<typename Pin::Input>;
	using IO = GpioInverted<typename Pin::IO>;
	using Type = typename Pin::Type;
	
    inline static bool isInverted = false;

    inline static void setInvert(bool invert) {
        isInverted = invert;
    }

	using Pin::setOutput;
	using Pin::set;
	using Pin::reset;
	using Pin::read;
	using Pin::isSet;

	inline static void
	setOutput(bool value)
	{
        if(isInverted) {
		    Pin::setOutput(!value);
        } else {
            Pin::setOutput(value);
        }
	}

	inline static void
	set()
	{
        if(isInverted) {
		    Pin::reset();
        } else {
            Pin::set();
        }
	}

	inline static void
	set(bool value)
	{
        if(isInverted) {
		    Pin::set(!value);
        } else {
            Pin::set(value);
        }
	}

	inline static void
	reset()
	{
        if(isInverted) {
		    Pin::set();
        } else {
            Pin::reset();
        }
	}

	inline static bool
	read()
	{
        if(isInverted) {
		    return !Pin::read();
        } else {
            return Pin::read();
        }
	}

	inline static bool
	isSet()
	{
        if(isInverted) {
		    return !Pin::isSet();
        } else {
            return Pin::isSet();
        }
	}
};
