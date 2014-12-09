package fr.grame.simpleguidoeditor.drawcommand;
import drawcommand.OffsetOriginCommand;

import android.graphics.Canvas;
import fr.grame.simpleguidoeditor.GuidoCanvasView;
   
public class AndroidCanvasOffsetOriginCommand extends OffsetOriginCommand implements DrawToCanvas {

  public AndroidCanvasOffsetOriginCommand(float x, float y) {
    super(x, y);
  }

  @Override
  public void drawToCanvas(Canvas canvas, GuidoCanvasView view) {
    int x = (int)(_x * view._GLOBAL_RESCALE_FACTOR);
    int y = (int)(_y * view._GLOBAL_RESCALE_FACTOR);

    canvas.translate(x, y);
  }
}