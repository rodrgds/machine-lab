import React from 'react';
import {
  AbsoluteFill,
  Audio,
  Easing,
  Img,
  OffthreadVideo,
  Sequence,
  interpolate,
  spring,
  staticFile,
  useCurrentFrame,
  useVideoConfig,
} from 'remotion';

const palette = {
  canvas: '#0b0e14',
  surface: '#141925',
  raised: '#1c2332',
  ink: '#f6f3ea',
  muted: '#aab3c5',
  blue: '#75a7ff',
  mint: '#57d8c7',
  amber: '#ffbf55',
  violet: '#ad8cff',
  coral: '#ff7d6e',
  green: '#67d391',
};

const clamp = {
  extrapolateLeft: 'clamp' as const,
  extrapolateRight: 'clamp' as const,
};

const ease = Easing.bezier(0.16, 1, 0.3, 1);

const fade = (frame: number, from: number, to: number) =>
  interpolate(frame, [from, to], [0, 1], {...clamp, easing: ease});

const fadeOut = (frame: number, from: number, to: number) =>
  interpolate(frame, [from, to], [1, 0], {...clamp, easing: ease});

const rise = (frame: number, delay = 0, distance = 54) =>
  interpolate(frame, [delay, delay + 24], [distance, 0], {...clamp, easing: ease});

const Scene: React.FC<{
  duration: number;
  accent: string;
  children: React.ReactNode;
}> = ({duration, accent, children}) => {
  const frame = useCurrentFrame();
  const opacity = Math.min(fade(frame, 0, 12), fadeOut(frame, duration - 12, duration));
  const gx = interpolate(Math.sin(frame / 38), [-1, 1], [20, 82]);
  const gy = interpolate(Math.cos(frame / 52), [-1, 1], [18, 78]);
  return (
    <AbsoluteFill
      style={{
        opacity,
        overflow: 'hidden',
        color: palette.ink,
        fontFamily: 'Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, sans-serif',
        background: `radial-gradient(circle at ${gx}% ${gy}%, ${accent}24, transparent 34%), linear-gradient(145deg, ${palette.canvas}, #101522 55%, #151725)`,
      }}
    >
      <AbsoluteFill
        style={{
          opacity: 0.14,
          backgroundImage:
            'linear-gradient(rgba(255,255,255,0.1) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px)',
          backgroundSize: '56px 56px',
          backgroundPosition: `${frame % 56}px ${frame % 56}px`,
          maskImage: 'radial-gradient(circle at center, black, transparent 82%)',
        }}
      />
      {children}
    </AbsoluteFill>
  );
};

const Eyebrow: React.FC<{children: React.ReactNode; color?: string}> = ({
  children,
  color = palette.mint,
}) => (
  <div
    style={{
      color,
      fontFamily: 'SFMono-Regular, Roboto Mono, Consolas, monospace',
      fontSize: 24,
      fontWeight: 800,
      letterSpacing: 2.5,
      textTransform: 'uppercase',
    }}
  >
    {children}
  </div>
);

const Heading: React.FC<{children: React.ReactNode; size?: number; width?: number}> = ({
  children,
  size = 102,
  width = 1380,
}) => (
  <div
    style={{
      maxWidth: width,
      fontSize: size,
      fontWeight: 900,
      lineHeight: 0.96,
      letterSpacing: -3,
      textWrap: 'balance',
    }}
  >
    {children}
  </div>
);

const Copy: React.FC<{children: React.ReactNode; width?: number}> = ({children, width = 980}) => (
  <div
    style={{
      maxWidth: width,
      color: palette.muted,
      fontSize: 34,
      lineHeight: 1.34,
      textWrap: 'balance',
    }}
  >
    {children}
  </div>
);

const Pill: React.FC<{children: React.ReactNode; color: string}> = ({children, color}) => (
  <div
    style={{
      border: `1px solid ${color}88`,
      borderRadius: 999,
      background: `${color}16`,
      color,
      padding: '13px 22px',
      fontSize: 24,
      fontWeight: 800,
      whiteSpace: 'nowrap',
    }}
  >
    {children}
  </div>
);

const WindowFrame: React.FC<{
  children: React.ReactNode;
  title: string;
  accent: string;
  width?: number;
}> = ({children, title, accent, width = 1160}) => (
  <div
    style={{
      width,
      borderRadius: 26,
      overflow: 'hidden',
      border: `1px solid ${accent}88`,
      background: palette.surface,
      boxShadow: `0 36px 120px ${accent}26`,
    }}
  >
    <div
      style={{
        height: 54,
        display: 'flex',
        alignItems: 'center',
        gap: 11,
        padding: '0 22px',
        background: '#202635',
        color: palette.muted,
        fontFamily: 'SFMono-Regular, Roboto Mono, Consolas, monospace',
        fontSize: 19,
      }}
    >
      {[palette.coral, palette.amber, palette.green].map((color) => (
        <div key={color} style={{width: 13, height: 13, borderRadius: 99, background: color}} />
      ))}
      <span style={{marginLeft: 10}}>{title}</span>
    </div>
    {children}
  </div>
);

const HookScene: React.FC = () => {
  const frame = useCurrentFrame();
  const {fps} = useVideoConfig();
  const pop = spring({frame, fps, config: {damping: 14, stiffness: 85}});
  return (
    <Scene duration={150} accent={palette.mint}>
      <div style={{position: 'absolute', left: 148, top: 150, display: 'flex', flexDirection: 'column', gap: 30}}>
        <div style={{opacity: fade(frame, 0, 18), transform: `translateY(${rise(frame)}px)`}}>
          <Eyebrow>Machine Lab</Eyebrow>
        </div>
        <div style={{transform: `scale(${0.94 + pop * 0.06})`, transformOrigin: 'left center'}}>
          <Heading size={126} width={1260}>Learn systems by building things that move.</Heading>
        </div>
        <Copy width={1100}>Portable C labs, virtual devices, deterministic feedback—and projects you can see and hear.</Copy>
        <div style={{display: 'flex', gap: 16, marginTop: 12}}>
          <Pill color={palette.blue}>C</Pill>
          <Pill color={palette.mint}>interrupts</Pill>
          <Pill color={palette.amber}>framebuffers</Pill>
          <Pill color={palette.violet}>PCM</Pill>
          <Pill color={palette.coral}>UART</Pill>
        </div>
      </div>
      <div
        style={{
          position: 'absolute',
          right: 128,
          bottom: 105,
          width: 585,
          padding: 28,
          borderRadius: 22,
          background: 'rgba(8, 12, 18, 0.92)',
          border: `1px solid ${palette.mint}55`,
          fontFamily: 'SFMono-Regular, Roboto Mono, Consolas, monospace',
          fontSize: 25,
          lineHeight: 1.65,
          transform: `translateY(${rise(frame, 18, 70)}px) rotate(-2deg)`,
          opacity: fade(frame, 16, 38),
        }}
      >
        <div style={{color: palette.muted}}>$ machinelab run project</div>
        <div style={{color: palette.green}}>devices ready</div>
        <div style={{color: palette.blue}}>framebuffer 800x600</div>
        <div style={{color: palette.violet}}>audio 48000 Hz stereo</div>
      </div>
    </Scene>
  );
};

const ContrastScene: React.FC = () => {
  const frame = useCurrentFrame();
  const columns = [
    {
      label: 'KEEP THE LEARNING',
      color: palette.mint,
      items: ['ports and registers', 'IRQs and event loops', 'pixels and PCM', 'serial protocols'],
    },
    {
      label: 'REMOVE THE FRICTION',
      color: palette.coral,
      items: ['first-week VM setup', 'Minix-only glue', 'manual-only grading', 'platform lock-in'],
    },
  ];
  return (
    <Scene duration={180} accent={palette.coral}>
      <div style={{position: 'absolute', left: 150, top: 112}}>
        <Eyebrow color={palette.coral}>The practical rewrite</Eyebrow>
        <Heading size={86}>Old PC ideas. Modern classroom setup.</Heading>
      </div>
      <div style={{position: 'absolute', left: 150, right: 150, bottom: 116, display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 34}}>
        {columns.map((column, columnIndex) => (
          <div
            key={column.label}
            style={{
              borderRadius: 28,
              padding: 34,
              background: 'rgba(255,255,255,0.055)',
              border: `1px solid ${column.color}66`,
              opacity: fade(frame, 12 + columnIndex * 12, 34 + columnIndex * 12),
              transform: `translateY(${rise(frame, 12 + columnIndex * 12, 42)}px)`,
            }}
          >
            <Eyebrow color={column.color}>{column.label}</Eyebrow>
            <div style={{display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 18, marginTop: 28}}>
              {column.items.map((item) => (
                <div key={item} style={{fontSize: 30, fontWeight: 750, padding: '22px 24px', borderRadius: 18, background: palette.surface}}>
                  {item}
                </div>
              ))}
            </div>
          </div>
        ))}
      </div>
    </Scene>
  );
};

const LabsScene: React.FC = () => {
  const frame = useCurrentFrame();
  const labs = [
    ['01', 'RTC', 'bits', palette.mint],
    ['02', 'PIT', 'time', palette.amber],
    ['03', 'KBD', 'keys', palette.blue],
    ['04', 'MOUSE', 'packets', palette.violet],
    ['05', 'VBE', 'pixels', palette.coral],
    ['06', 'PCM', 'sound', palette.green],
    ['07', 'UART', 'bytes', '#ff9f7a'],
  ];
  return (
    <Scene duration={150} accent={palette.blue}>
      <div style={{position: 'absolute', left: 150, top: 130}}>
        <Eyebrow color={palette.blue}>Seven guided labs</Eyebrow>
        <Heading size={82}>One device at a time. One real artifact at a time.</Heading>
      </div>
      <div style={{position: 'absolute', left: 130, right: 130, bottom: 150, display: 'grid', gridTemplateColumns: 'repeat(7, 1fr)', gap: 16}}>
        {labs.map(([number, name, detail, color], index) => (
          <div
            key={name}
            style={{
              height: 315,
              borderRadius: 24,
              padding: 25,
              border: `1px solid ${color}70`,
              background: `linear-gradient(180deg, ${color}22, rgba(255,255,255,0.045))`,
              opacity: fade(frame, index * 7, index * 7 + 18),
              transform: `translateY(${rise(frame, index * 7, 56)}px)`,
            }}
          >
            <div style={{fontFamily: 'monospace', color, fontSize: 24}}>{number}</div>
            <div style={{fontSize: 38, fontWeight: 900, marginTop: 124}}>{name}</div>
            <div style={{fontSize: 24, color: palette.muted, marginTop: 10}}>{detail}</div>
          </div>
        ))}
      </div>
    </Scene>
  );
};

const WordScene: React.FC = () => {
  const frame = useCurrentFrame();
  return (
    <Scene duration={240} accent={palette.blue}>
      <div style={{position: 'absolute', left: 115, top: 102, width: 545, display: 'flex', flexDirection: 'column', gap: 28}}>
        <Eyebrow color={palette.blue}>Build real tools</Eyebrow>
        <Heading size={84} width={560}>A word processor from scancodes and pixels.</Heading>
        <Copy width={520}>Case handling, cursor movement, wrapping, scrolling, save, undo, and a framebuffer UI.</Copy>
        <div style={{display: 'flex', flexWrap: 'wrap', gap: 12}}>
          <Pill color={palette.blue}>keyboard IRQ</Pill>
          <Pill color={palette.mint}>editor state</Pill>
          <Pill color={palette.amber}>VBE</Pill>
        </div>
      </div>
      <div style={{position: 'absolute', right: 94, top: 130, transform: `translateX(${interpolate(frame, [0, 28], [100, 0], {...clamp, easing: ease})}px)`, opacity: fade(frame, 0, 20)}}>
        <WindowFrame title="machinelab run word_processor" accent={palette.blue} width={1140}>
          <OffthreadVideo
            src={staticFile('clips/word-processor.mp4')}
            muted
            style={{display: 'block', width: '100%', aspectRatio: '4 / 3', objectFit: 'cover'}}
          />
        </WindowFrame>
      </div>
    </Scene>
  );
};

const MusicScene: React.FC = () => {
  const frame = useCurrentFrame();
  const bars = [0.35, 0.72, 0.48, 0.9, 0.55, 0.78, 0.42, 0.84];
  return (
    <Scene duration={270} accent={palette.violet}>
      <div style={{position: 'absolute', left: 92, top: 126}}>
        <WindowFrame title="machinelab run music_maker" accent={palette.violet} width={1180}>
          <OffthreadVideo
            src={staticFile('clips/music-maker.mp4')}
            muted
            style={{display: 'block', width: '100%', aspectRatio: '4 / 3', objectFit: 'cover'}}
          />
        </WindowFrame>
      </div>
      <div style={{position: 'absolute', right: 112, top: 145, width: 520, display: 'flex', flexDirection: 'column', gap: 26}}>
        <Eyebrow color={palette.violet}>Hear every step, on time</Eyebrow>
        <Heading size={78} width={540}>A pocket composer—not a sound dump.</Heading>
        <Copy width={510}>Four instruments, a chromatic piano roll, mute and tempo controls, looping transport, and real PCM output.</Copy>
        <div style={{height: 120, display: 'flex', gap: 12, alignItems: 'flex-end', marginTop: 8}}>
          {bars.map((height, index) => {
            const pulse = 0.65 + 0.35 * Math.sin((frame + index * 8) / 8);
            return (
              <div
                key={index}
                style={{
                  width: 38,
                  height: `${height * pulse * 100}%`,
                  borderRadius: 8,
                  background: [palette.amber, palette.mint, palette.violet, palette.coral][index % 4],
                }}
              />
            );
          })}
        </div>
      </div>
    </Scene>
  );
};

const MontageScene: React.FC = () => {
  const frame = useCurrentFrame();
  const shots = [
    ['breakout.png', 'ARCADE LOOP', palette.amber],
    ['flappy.png', 'REPLAYABLE GAME', palette.green],
    ['ninjix.png', 'UART PROJECT', palette.coral],
  ];
  return (
    <Scene duration={180} accent={palette.amber}>
      <div style={{position: 'absolute', left: 150, top: 104}}>
        <Eyebrow color={palette.amber}>Then scale up</Eyebrow>
        <Heading size={84}>From device lab to final project.</Heading>
      </div>
      <div style={{position: 'absolute', left: 96, right: 96, bottom: 105, display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 24}}>
        {shots.map(([image, label, color], index) => (
          <div
            key={image}
            style={{
              borderRadius: 24,
              overflow: 'hidden',
              border: `1px solid ${color}70`,
              background: palette.surface,
              boxShadow: `0 25px 80px ${color}20`,
              opacity: fade(frame, index * 10, index * 10 + 20),
              transform: `translateY(${rise(frame, index * 10, 48)}px) rotate(${index - 1}deg)`,
            }}
          >
            <Img src={staticFile(`screenshots/${image}`)} style={{display: 'block', width: '100%', imageRendering: 'pixelated'}} />
            <div style={{padding: '22px 26px', fontSize: 25, fontWeight: 850, color}}>{label}</div>
          </div>
        ))}
      </div>
    </Scene>
  );
};

const WorkflowScene: React.FC = () => {
  const frame = useCurrentFrame();
  const steps = [
    ['01', 'SETUP', 'generated workspace', palette.blue],
    ['02', 'FAIL', 'focused lab test', palette.coral],
    ['03', 'INSPECT', 'trace + artifact', palette.amber],
    ['04', 'PASS', 'deterministic check', palette.green],
    ['05', 'SHIP', 'replay + bundle', palette.violet],
  ];
  return (
    <Scene duration={180} accent={palette.green}>
      <div style={{position: 'absolute', left: 150, top: 118}}>
        <Eyebrow color={palette.green}>The feedback loop</Eyebrow>
        <Heading size={88}>No black box. Every failure leaves evidence.</Heading>
      </div>
      <div style={{position: 'absolute', left: 120, right: 120, bottom: 145, display: 'grid', gridTemplateColumns: 'repeat(5, 1fr)', gap: 18}}>
        {steps.map(([number, title, detail, color], index) => (
          <div key={title} style={{position: 'relative', opacity: fade(frame, 8 + index * 9, 24 + index * 9), transform: `translateY(${rise(frame, 8 + index * 9, 44)}px)`}}>
            <div style={{height: 260, padding: 28, borderRadius: 24, border: `1px solid ${color}66`, background: 'rgba(255,255,255,0.055)'}}>
              <div style={{fontFamily: 'monospace', color, fontSize: 25}}>{number}</div>
              <div style={{fontSize: 38, fontWeight: 900, marginTop: 92}}>{title}</div>
              <div style={{fontSize: 23, color: palette.muted, marginTop: 10}}>{detail}</div>
            </div>
            {index < steps.length - 1 ? <div style={{position: 'absolute', right: -17, top: 128, color, fontSize: 28}}>›</div> : null}
          </div>
        ))}
      </div>
    </Scene>
  );
};

const CourseScene: React.FC = () => {
  const frame = useCurrentFrame();
  return (
    <Scene duration={120} accent={palette.blue}>
      <div style={{position: 'absolute', inset: '130px 150px', display: 'grid', gridTemplateColumns: '1.1fr 0.9fr', gap: 90, alignItems: 'center'}}>
        <div style={{display: 'flex', flexDirection: 'column', gap: 28}}>
          <Eyebrow color={palette.blue}>Ready to teach</Eyebrow>
          <Heading size={94}>A course, not just a runtime.</Heading>
          <Copy>Student guides, instructor notes, lab handouts, predefined checks, rubrics, releases, and project examples.</Copy>
        </div>
        <div style={{display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 18}}>
          {['7 labs', '6 ECTS shape', '4 platforms', 'MIT + CC'].map((label, index) => (
            <div
              key={label}
              style={{
                height: 150,
                borderRadius: 22,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                border: `1px solid ${[palette.blue, palette.mint, palette.amber, palette.green][index]}66`,
                background: 'rgba(255,255,255,0.055)',
                fontSize: 32,
                fontWeight: 850,
                opacity: fade(frame, index * 7, index * 7 + 18),
              }}
            >
              {label}
            </div>
          ))}
        </div>
      </div>
    </Scene>
  );
};

const ClosingScene: React.FC = () => {
  const frame = useCurrentFrame();
  return (
    <Scene duration={150} accent={palette.mint}>
      <div style={{position: 'absolute', inset: 0, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', textAlign: 'center', gap: 30}}>
        <Eyebrow>Machine Lab</Eyebrow>
        <Heading size={112} width={1380}>Teach the machine-facing parts. Keep the joy of making.</Heading>
        <div
          style={{
            marginTop: 24,
            borderRadius: 18,
            padding: '20px 30px',
            background: 'rgba(7,11,16,0.82)',
            border: `1px solid ${palette.mint}66`,
            fontFamily: 'SFMono-Regular, Roboto Mono, Consolas, monospace',
            fontSize: 29,
            color: palette.mint,
            opacity: fade(frame, 24, 48),
            transform: `translateY(${rise(frame, 24, 32)}px)`,
          }}
        >
          machine-lab-docs.pages.dev
        </div>
      </div>
    </Scene>
  );
};

export const MachineLabAnnouncement: React.FC = () => (
  <AbsoluteFill style={{background: palette.canvas}}>
    <Audio src={staticFile('audio/music-maker.wav')} loop volume={0.65} />
    <Sequence from={0} durationInFrames={150}><HookScene /></Sequence>
    <Sequence from={150} durationInFrames={180}><ContrastScene /></Sequence>
    <Sequence from={330} durationInFrames={150}><LabsScene /></Sequence>
    <Sequence from={480} durationInFrames={240}><WordScene /></Sequence>
    <Sequence from={720} durationInFrames={270}><MusicScene /></Sequence>
    <Sequence from={990} durationInFrames={180}><MontageScene /></Sequence>
    <Sequence from={1170} durationInFrames={180}><WorkflowScene /></Sequence>
    <Sequence from={1350} durationInFrames={120}><CourseScene /></Sequence>
    <Sequence from={1470} durationInFrames={150}><ClosingScene /></Sequence>
  </AbsoluteFill>
);
