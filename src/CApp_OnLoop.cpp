//==============================================================================
#include "CApp.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "font.h"

bool CApp::FreeRect(Geometry::Vector2d<int> p, Geometry::Vector2d<int> s)
{
	Geometry::Vector2d<int> d(Geometry::uninitialised);
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

Geometry::Vector2d<int> CApp::SpawnPoint(Geometry::Vector2d<int> exclude)
{
	const int s=3;
	Geometry::Vector2d<int> result( Geometry::uninitialised );
	do{
		result = { mRNG()%(mHorizontal-s), 1 + mRNG()%(mVertical-(s+8)) };
	}while( !FreeRect( result, {s,s} ) || result.DistanceSquare(exclude)<=(8+s) );
	return result;
}

void CApp::Spawn(Geometry::Vector2d<int> e, int i)
{
	Geometry::Vector2d<int> p = SpawnPoint(e);
	if (i==2)
	{
		*GetPx(p+Geometry::Vector2d<int>(1,0))=i;
		*GetPx(p+Geometry::Vector2d<int>(0,1))=i;	
		*GetPx(p+Geometry::Vector2d<int>(1,1))=i;
		*GetPx(p+Geometry::Vector2d<int>(2,1))=i;
		*GetPx(p+Geometry::Vector2d<int>(1,2))=i;	
	}
	else
	{
		*GetPx(p+Geometry::Vector2d<int>(0,0))=i;
		*GetPx(p+Geometry::Vector2d<int>(0,2))=i;
		*GetPx(p+Geometry::Vector2d<int>(1,1))=i;
		*GetPx(p+Geometry::Vector2d<int>(2,0))=i;
		*GetPx(p+Geometry::Vector2d<int>(2,2))=i;
	}
}

void CApp::PrintNumber( int num, int h, int v, bool ralign)	
{
	using namespace Font;
	
	char buffer[8];
	int l=sprintf(buffer,"%i", num);
	int x = 1;
	if (ralign) x = h - l*4;
	for (int i = 0 ; i!=l; ++i)
	{
		x+=Blit(GetDigit(buffer[i]-'0'), x, v, mPixels, mHorizontal, mVertical);
	}
}

int CApp::Consume( Geometry::Vector2d<int>::BaseType pos )
{
	int result = 0;
	int* p = GetPx( pos );
	if (*p==2)
	{
		*p=0;
		result ++;
		result += Consume( pos + Geometry::Vector2d<int>(1,0) );
		result += Consume( pos + Geometry::Vector2d<int>(0,1) );
		result += Consume( pos + Geometry::Vector2d<int>(-1,0) );
		result += Consume( pos + Geometry::Vector2d<int>(0,-1) );
	}
	return result;
}

// returns true if the snake lives on, false is death
bool CApp::Occupy( Geometry::Vector2d<int> pos )
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
			Spawn(mPos.front(),2);
			mPendingGrowth += Consume(pos) * 2;
		}
		
		*p = 1;
		
		return true;
	}
}

Geometry::Vector2d<int> Other(Geometry::Vector2d<int> pos, Geometry::Vector2d<int> dir)
{
	Geometry::Vector2d<int> other = pos;
	
	if (dir[1]==0) other[1]++;
	else other[0]++;
	
	return other;
}

void CApp::AdvanceTail()
{
	*GetPx( mPos.back() ) = 0;
	mPos.pop_back();
	if (mOther.empty()==false)
	{
		*GetPx( mOther.back() ) = 0;
		mOther.pop_back();
	}
}

//==============================================================================
void CApp::OnLoop() 
{
	static int lastscore=1;
	static int deadFrame = 0;

	mLoopCount++;
	
	int score=mPos.size();
	
	// clear score area
	std::fill(mPixels+(mHorizontal*(mVertical-7)), mPixels+(mHorizontal*mVertical),0);
	// score area horizontal
	std::fill(mPixels+(mHorizontal*(mVertical-8)), mPixels+(mHorizontal*(mVertical-7)),1);
	// bottom border horizontal
	std::fill(mPixels, mPixels+mHorizontal,1);

	static int best = 0;
	if (score>best) best=score;

	PrintNumber( deadFrame==0 ? score : lastscore, 1, mVertical-2, false );
	PrintNumber( best, mHorizontal, mVertical-2, true );

	static Geometry::VectorN<int,2> latchDir {0,0};
	static int latch = 0;
	if (latch) latch--;
	if (!mEvents.empty() && (latch==0 || mEvents.front()!=latchDir)) 
	{
		// a change in direction, not reversing direction
		if (mEvents.front()!=-mDir && mEvents.front()!=mDir)
		{
			auto oldDir = mDir;
			mDir = mEvents.front();
			
			// step over thickness
			if (mDir[1]==1 || mDir[0]==1) 
			{
				mPos.push_front( Geometry::Vector2d<int>(mPos.front() + mDir) );
				
				Geometry::Vector2d<int> other = Other(mPos.front(), mDir);
				if (mOther.empty() || other!=mOther.front())
					mOther.push_front( other );
				
				AdvanceTail();
			}
			if (oldDir[1]==1 || oldDir[0]==1) 
			{
				mPos.push_front( Geometry::Vector2d<int>(mPos.front() - oldDir) );
				
				Geometry::Vector2d<int> other = Other(mPos.front(), mDir);
				if (mOther.empty() || other!=mOther.front())
					mOther.push_front( other );
					
				AdvanceTail();
			}
			latchDir=-oldDir;
			latch=2;
		}
		mEvents.pop_front();
	}
	
	if (mDir.LengthSquare()>0 && !deadFrame)
	{
		// constantly spawn bad spots
		if (mSpawnCooldown>0)
		{
			mSpawnCooldown--;
		}
		else if (mRNG()%12 == 0)
		{
			Spawn(mPos.front(),1);
			mSpawnCooldown = 12;
		}
		
		Geometry::Vector2d<int> next( mPos.front() + mDir );
		if (next[0]>=mHorizontal) next[0] -= mHorizontal;
		if (next[0]<0) next[0] += mHorizontal;
		if (next[1]>=mVertical) next[1] -= mVertical;
		if (next[1]<0) next[1] += mVertical;
		mPos.push_front( next );
		
		Geometry::Vector2d<int> other = Other(next,mDir);
		
		if (mOther.empty() || other!=mOther.front())
			mOther.push_front( other );
		
		if (!Occupy(next) || !Occupy(other))
		{
			lastscore = score;
			score = 0;
			deadFrame = mLoopCount;
			mDir = {0,0};
		}


		// grow if growing		
		if (mPendingGrowth>0)
		{
			mPendingGrowth--;
		}
		else
		{
			// clear up tail
			AdvanceTail();
		}		
	}
	else if (deadFrame) 
	{
		if (mLoopCount >= deadFrame+16)
		{
			Reset();
			deadFrame = 0;
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

//==============================================================================
