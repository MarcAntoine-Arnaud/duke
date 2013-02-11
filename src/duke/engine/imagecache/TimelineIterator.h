/*
 * TimelineIterator.h
 *
 *  Created on: Feb 5, 2013
 *      Author: Guillaume Chatelet
 */

#ifndef TIMELINEITERATOR_H_
#define TIMELINEITERATOR_H_

#include <duke/engine/Timeline.h>

namespace duke {

Ranges getMediaRanges(const Timeline &timeline);
bool contains(const Ranges &range, size_t frame);

struct TimelineIterator {
	TimelineIterator();
	TimelineIterator(const Timeline * pTimeline, const Ranges *pMediaRanges, size_t currentFrame);
	void clear();
	MediaFrameReference next();
	bool empty();
	size_t getCurrentFrame() const;
private:
	void regularizeCurrentFrame();
	void stepForward();
	void stepUntilValidOrExhausted();
	bool valid() const;
	const Track& getCurrentTrack() const;

	const Timeline * m_pTimeline;
	const Ranges *m_pMediaRanges;
	size_t m_CurrentFrame;
	size_t m_CurrentTrackIndex;
	size_t m_EndFrame;
};

struct LimitedTimelineIterator {
	LimitedTimelineIterator();
	LimitedTimelineIterator(const Timeline * pTimeline, const Ranges *pMediaRanges, size_t currentFrame, size_t iterations);
	void clear();
	MediaFrameReference next();
	bool empty();
	size_t getCurrentFrame() const;
private:
	TimelineIterator m_Iterator;
	size_t m_IterationToEnd;
};

} /* namespace duke */
#endif /* TIMELINEITERATOR_H_ */