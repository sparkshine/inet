#include "Decider.h"

const_simtime_t Decider::notAgain(-1);

bool DeciderResult::isSignalCorrect() const {
	return isCorrect;
}

Decider::Decider(DeciderToPhyInterface* phy):
	phy(phy)
{}

simtime_t Decider::processSignal(DetailedRadioFrame* /*s*/)
{
	return notAgain;
}

ChannelState Decider::getChannelState() const {
	return ChannelState();
}
