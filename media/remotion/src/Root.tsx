import {Composition} from 'remotion';
import {MachineLabAnnouncement} from './Video';

export const DURATION_IN_FRAMES = 1620;
export const FPS = 30;
export const WIDTH = 1920;
export const HEIGHT = 1080;

export const RemotionRoot = () => {
  return (
    <Composition
      id="MachineLabAnnouncement"
      component={MachineLabAnnouncement}
      durationInFrames={DURATION_IN_FRAMES}
      fps={FPS}
      width={WIDTH}
      height={HEIGHT}
    />
  );
};
