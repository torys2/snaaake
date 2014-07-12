#include "snaaake.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include <stdio.h>
#include <stdlib.h>

SnakeApp::SnakeApp()
	// set up in ::Init
	: mDir( Geometry::uninitialised )
	, mPendingRemove( Geometry::uninitialised )
	, mPaused(false)
{
}

void SnakeApp::Reset() 
{
	mPos.erase(mPos.begin(), mPos.end());
	mOther.erase(mOther.begin(), mOther.end());
	
	Geometry::Vector2d<int> pos = { mHorizontal/2, mVertical/2 };
	mPos.push_back(pos);
	
	Clear(0);
	*GetPx(pos) = 1;

	Spawn(2);
	Spawn(2);
		
	mPendingGrowth = 1;
	mSpawnCooldown = 12;
	mDir[0] = 0;
	mDir[1] = 0;
	
	mScore = 0;
	mDiedOnFrame = 0;
}

bool SnakeApp::OnInit()
{
 	printf("Snake.\nControls: WASD or Arrow Keys.\nSpace to Pause.\n");
    bool result = Base::OnInit();
    if (result)
    {
    	Reset();
	    mHighScore=0;
    }
    return result;
}

void SnakeApp::OnEvent(SDL_Event* Event)
{
	Base::OnEvent(Event);
	if (Event->type==SDL_KEYDOWN)
    {
    	switch (Event->key.keysym.sym)
    	{
    		case SDLK_a:
    		case SDLK_LEFT:
		    	mEvents.push_back({-1,0});
				break;
			case SDLK_d:
    		case SDLK_RIGHT:
		    	mEvents.push_back({1,0});
				break;
			case SDLK_w:
    		case SDLK_UP:
		    	mEvents.push_back({0,1});
		    	break;
			case SDLK_s:
    		case SDLK_DOWN:
		    	mEvents.push_back({0,-1});
				break;
			case SDLK_SPACE:
				mPaused = !mPaused;
				break;
			default:
				break;
		}
    }	
}

bool SnakeApp::FreeRect(Point p, Point s)
{
	Point d(Geometry::uninitialised);
	for (d[0]=0;d[0]!=s[0];++d[0])
	{
		for (d[1]=0;d[1]!=s[1];++d[1])
		{
			if (*GetPx(p+d))
				return false;
		}
	}
	return true;
}

int SnakeApp::SpawnDistance(Point p)
{
	// spawns are 3x3 and p is on the 0,0,
	// so measure distance from centre of spawn
	p += {1,1};
	int r = p.ManhattenDistance(mPos.front());
	if (mOther.empty()==false)
		r = std::min(r, p.ManhattenDistance(mOther.front()));
	return r;
}

CApp::Point SnakeApp::SpawnPoint()
{
	// 3x3 spawns, plus 1px space around
	const int s=3+2;
	Point result( Geometry::uninitialised );
	do{
		int x = mRNG()%(mHorizontal-s);
		int y = 1 + mRNG()%(mVertical-(s+8));
		result = { x, y };
	}while( !FreeRect( result, {s,s} ) || SpawnDistance(result)<=(16+s) );
	result += {1,1};
	return result;
}

void SnakeApp::Spawn(int i)
{
	Point p = SpawnPoint();
	if (i==2)
	{
		*GetPx(p+Point(1,0))=i;
		*GetPx(p+Point(0,1))=i;	
		*GetPx(p+Point(1,1))=i;
		*GetPx(p+Point(2,1))=i;
		*GetPx(p+Point(1,2))=i;	
	}
	else
	{
		*GetPx(p+Point(0,0))=i;
		*GetPx(p+Point(0,2))=i;
		*GetPx(p+Point(1,1))=i;
		*GetPx(p+Point(2,0))=i;
		*GetPx(p+Point(2,2))=i;
	}
}

int SnakeApp::Consume( Point::BaseType pos )
{
	int result = 0;
	int* p = GetPx( pos );
	if (*p==2)
	{
		*p=0;
		result ++;
		result += Consume( pos + Point(1,0) );
		result += Consume( pos + Point(0,1) );
		result += Consume( pos + Point(-1,0) );
		result += Consume( pos + Point(0,-1) );
	}
	return result;
}

// returns true if the snake lives on, false is death
bool SnakeApp::Occupy( Point pos )
{
	int* p = GetPx( pos );
	if (*p==1)
	{
		return false;
	}
	else
	{
		if (*p==2)
		{
			Spawn(2);
			mPendingGrowth += Consume(pos) * 2;
			mSpawnCooldown = 12;
			mPendingRemove = mPos.front();
		}
		
		*p = 1;
		
		return true;
	}
}

CApp::Point SnakeApp::Other(
	Point pos, 
	Point dir
)
{
	Point other = pos;
	
	if (dir[1]==0) other[1]++;
	else other[0]++;
	
	return Wrap(other);
}

void SnakeApp::AdvanceTail()
{
	*GetPx( mPos.back() ) = 0;
	mPos.pop_back();
	if (mOther.empty()==false)
	{
		*GetPx( mOther.back() ) = 0;
		mOther.pop_back();
	}
}

CApp::Point SnakeApp::RemoveSpawn(int c, Point near)
{
	Point p(0,0), best(0,0);
	bool first = true;
	
    for (p[0]=0;p[0]!=mHorizontal;p[0]++)
    {
	    for (p[1]=1;p[1]!=mVertical-8;p[1]++)
    	{
    		int i = *GetPx( p );
    		if (i==c)
    		{
    			if (std::find(mPos.begin(), mPos.end(), p)==mPos.end())
    			{
					if (std::find(mOther.begin(), mOther.end(), p)==mOther.end())
					{
						if (first || near.DistanceSquare(p)<near.DistanceSquare(best))
						{
							best=p;
							first=false;
						}
					}
    			}
    		}
    	}
    }
    *GetPx( best ) = 0;
    return best;
}

CApp::Point SnakeApp::Wrap(const Point::BaseType& _p)const
{
	Point p(_p);
	if (p[0]>=mHorizontal) p[0] -= mHorizontal;
	if (p[0]<0) p[0] += mHorizontal;
	if (p[1]>=mVertical) p[1] -= mVertical;
	if (p[1]<0) p[1] += mVertical;
	return p;
}

CApp::Point SnakeApp::Advance(
	const Point& p,
	const Point::BaseType& d)
{
	Point next( p + d );
	return Wrap(next);
}

//==============================================================================
void SnakeApp::OnLoop() 
{
	mLoopCount++;
	
	mScore=std::max(static_cast<int>(mPos.size())-1,mScore);
	
	// clear score area
	ClearRows(mVertical-7, mVertical, 0);
	// score area horizontal
	ClearRows(mVertical-8, mVertical-7, 1);
	// bottom border horizontal
	ClearRows(0, 1, 1);

	mHighScore=std::max(mScore, mHighScore);

	PrintNumber( mScore, 1, mVertical-2, Left );
	PrintNumber( mHighScore, mHorizontal, mVertical-2, Right );
	
	if (mPaused)
	{
		// all game input events are dropped while paused
		if (!mEvents.empty()) mEvents.pop_front();
		// on screen message during pause
		PrintString( "PAUSED", mHorizontal/2, mVertical-2, Center );		
	}
	else
	{
		PrintString( "SNAAAKE", mHorizontal/2, mVertical-2, Center );		
		mInputLatch.Update();
		if (!mEvents.empty() && !mInputLatch.IsLocked(mEvents.front())) 
		{
			// a change in direction, not reversing direction
			if (mEvents.front()!=-mDir && mEvents.front()!=mDir)
			{
				auto oldDir = mDir;
				mDir = mEvents.front();
			
				// step over thickness
				if (mDir[1]==1 || mDir[0]==1) 
				{
					mPos.push_front( Advance( mPos.front(), mDir ) );
				
					Point other = Other(mPos.front(), mDir);
					if (mOther.empty() || other!=mOther.front())
						mOther.push_front( other );
				
					AdvanceTail();
				}
				if (oldDir[1]==1 || oldDir[0]==1) 
				{
					mPos.push_front( Advance( mPos.front(), -oldDir ) );
				
					Point other = Other(mPos.front(), mDir);
					if (mOther.empty() || other!=mOther.front())
					{
						if (mOther.empty()==false)
						{
							if (*GetPx(mOther.front())==0)
							{
								mOther.pop_front();
								mOther.push_front( other );
							}
						}
						mOther.push_front( other );
					}
					
					AdvanceTail();
				}
				mInputLatch.Lock(2, -oldDir);
			}
			mEvents.pop_front();
		}
	
		if (mDir.LengthSquare()>0 && !mDiedOnFrame)
		{
			// constantly spawn bad spots
			if (mSpawnCooldown>0)
			{
				mSpawnCooldown--;
			}
			else if (mPendingGrowth==0 && (mRNG()%12)==0)
			{
				Spawn(1);
				mSpawnCooldown = 12;
			}
		
			Point next = Advance( mPos.front(), mDir );
			mPos.push_front( next );
		
			Point other = Other(next,mDir);
		
			if (mOther.empty() || other!=mOther.front())
				mOther.push_front( other );
		
			if (!Occupy(next) || !Occupy(other))
			{
				mDiedOnFrame = mLoopCount;
				mDir = {0,0};
			}


			// grow if growing		
			if (mPendingGrowth>0)
			{
				mPendingGrowth--;
				mPendingRemove = RemoveSpawn(1, mPendingRemove);
			}
			else
			{
				// clear up tail
				AdvanceTail();
			}		
		}
		else if (mDiedOnFrame) 
		{
			if (mLoopCount >= mDiedOnFrame+16)
			{
				Reset();
			}
			else 
			{
				for (int i=0;i!=mPos.size();++i)
					*GetPx( mPos[i] ) = mLoopCount%2;
				for (int i=0;i!=mOther.size();++i)
					*GetPx( mOther[i] ) = mLoopCount%2;
			}
		}
	}
}